#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/sysctl.h>
#include <libproc.h>
#include <pwd.h>
#include <utmpx.h>
#include <mach/mach.h>

#include <arpa/inet.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <net/route.h>

#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOBSD.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/storage/IOBlockStorageDriver.h>
#include <IOKit/storage/IOMedia.h>

#include "pslib.h"
#include "common.h"

#define TV2DOUBLE(t) ((t).tv_sec + (t).tv_usec / 1000000.0)

/* Internal functions */

static CpuTimes *per_cpu_times() {
  CpuTimes *ret = NULL;

  natural_t cpu_count;
  processor_info_array_t info_array;
  mach_msg_type_number_t info_count;
  kern_return_t kerror;
  processor_cpu_load_info_data_t *cpu_load_info = NULL;
  kern_return_t sysret;
  int32_t pagesize = getpagesize();

  mach_port_t host_port = mach_host_self();
  kerror = host_processor_info(host_port, PROCESSOR_CPU_LOAD_INFO, &cpu_count,
                               &info_array, &info_count);
  check(kerror == KERN_SUCCESS, "Error in host_processor_info(): %s",
        mach_error_string(kerror));
  mach_port_deallocate(mach_task_self(), host_port);

  cpu_load_info = (processor_cpu_load_info_data_t *)info_array;

  ret = (CpuTimes *)calloc(cpu_count, sizeof(CpuTimes));
  check_mem(ret);

  for (natural_t i = 0; i < cpu_count; i++) {
    (ret + i)->user =
        (double)cpu_load_info[i].cpu_ticks[CPU_STATE_USER] / CLK_TCK;
    (ret + i)->nice =
        (double)cpu_load_info[i].cpu_ticks[CPU_STATE_NICE] / CLK_TCK;
    (ret + i)->system =
        (double)cpu_load_info[i].cpu_ticks[CPU_STATE_SYSTEM] / CLK_TCK;
    (ret + i)->idle =
        (double)cpu_load_info[i].cpu_ticks[CPU_STATE_IDLE] / CLK_TCK;
  }

  sysret = vm_deallocate(mach_task_self(), (vm_address_t)info_array,
                         info_count * pagesize);
  if (sysret != KERN_SUCCESS)
    log_warn("vm_deallocate() failed");

  return ret;

error:
  free(ret);
  if (cpu_load_info != NULL) {
    sysret = vm_deallocate(mach_task_self(), (vm_address_t)info_array,
                           info_count * pagesize);
    if (sysret != KERN_SUCCESS)
      log_warn("vm_deallocate() failed");
  }
  return NULL;
}

static double sum_cpu_time(CpuTimes *t) {
  double ret = 0.0;
  ret += t->user;
  ret += t->system;
  ret += t->idle;
  ret += t->nice;
  ret += t->iowait;
  ret += t->irq;
  ret += t->softirq;
  ret += t->steal;
  ret += t->guest;
  ret += t->guest_nice;
  return ret;
}

static CpuTimes *calculate_cpu_times_percentage(CpuTimes *t1, CpuTimes *t2) {
  CpuTimes *ret;
  double all_delta = sum_cpu_time(t2) - sum_cpu_time(t1);
  ret = (CpuTimes *)calloc(1, sizeof(CpuTimes));
  check_mem(ret);

  ret->user = 100 * (t2->user - t1->user) / all_delta;
  ret->system = 100 * (t2->system - t1->system) / all_delta;
  ret->idle = 100 * (t2->idle - t1->idle) / all_delta;
  ret->nice = 100 * (t2->nice - t1->nice) / all_delta;
  ret->iowait = 100 * (t2->iowait - t1->iowait) / all_delta;
  ret->irq = 100 * (t2->irq - t1->irq) / all_delta;
  ret->softirq = 100 * (t2->softirq - t1->softirq) / all_delta;
  ret->steal = 100 * (t2->steal - t1->steal) / all_delta;
  ret->guest = 100 * (t2->guest - t1->guest) / all_delta;
  ret->guest_nice = 100 * (t2->guest_nice - t1->guest_nice) / all_delta;
  return ret;

error:
  free(ret);
  return NULL;
}

static double calculate_cpu_util_percentage(CpuTimes *t1, CpuTimes *t2) {
  double t1_all = sum_cpu_time(t1);
  double t2_all = sum_cpu_time(t2);
  double t1_busy = t1_all - t1->idle;
  double t2_busy = t2_all - t2->idle;
  double busy_delta, all_delta, busy_percentage;

  /* This exists in psutils. We'll put it in if we actually find it */
  /* if (t2_busy < t1_busy)  */
  /*   return 0.0; /\* Indicates a precision problem *\/ */

  busy_delta = t2_busy - t1_busy;
  all_delta = t2_all - t1_all;
  busy_percentage = (busy_delta / all_delta) * 100;
  return busy_percentage;
}

/*
 * A wrapper around host_statistics() invoked with HOST_VM_INFO.
 */
static bool sys_vminfo(vm_statistics_data_t *vmstat) {
  kern_return_t ret;
  mach_msg_type_number_t count = sizeof(*vmstat) / sizeof(integer_t);
  mach_port_t mport = mach_host_self();

  ret = host_statistics(mport, HOST_VM_INFO, (host_info_t)vmstat, &count);
  if (ret != KERN_SUCCESS) {
    log_err("host_statistics() failed: %s", mach_error_string(ret));
    return false;
  }
  mach_port_deallocate(mach_task_self(), mport);
  return true;
}

static bool get_kinfo_proc(pid_t pid, struct kinfo_proc *kp) {
  int32_t mib[4];
  size_t len;
  mib[0] = CTL_KERN;
  mib[1] = KERN_PROC;
  mib[2] = KERN_PROC_PID;
  mib[3] = pid;

  // fetch the info with sysctl()
  len = sizeof(struct kinfo_proc);

  // now read the data from sysctl
  if (sysctl(mib, 4, kp, &len, NULL, 0) == -1) {
    // throw errno as the error
    log_err("");
    return false;
  }

  // sysctl succeeds but len is zero, happens when process has gone away
  check(len != 0, "No such process");
  return true;
error:
  return false;
}

static pid_t get_ppid(pid_t pid) {
  struct kinfo_proc kp;

  if (!get_kinfo_proc(pid, &kp))
    return -1;

  return (pid_t)kp.kp_eproc.e_ppid;
}

static char *get_procname(pid_t pid) {
  struct kinfo_proc kp;

  if (!get_kinfo_proc(pid, &kp))
    return NULL;

  return strdup(kp.kp_proc.p_comm);
}

static char *get_exe(pid_t pid) {
  char buf[PATH_MAX];
  int32_t ret;

  ret = proc_pidpath(pid, &buf, sizeof(buf));
  if (ret == 0) {
    if (!pid_exists(pid))
      log_err("No such process");
    else
      log_err("Access denied");
    return NULL;
  }
  return strdup(buf);
}

// Read the maximum argument size for processes
int32_t get_argmax() {
  int32_t argmax;
  int32_t mib[] = {CTL_KERN, KERN_ARGMAX};
  size_t size = sizeof(argmax);

  if (sysctl(mib, 2, &argmax, &size, NULL, 0) == 0)
    return argmax;
  return 0;
}

static char *get_cmdline(pid_t pid) {
  int32_t mib[3];
  int32_t nargs;
  int32_t len;
  char *procargs = NULL;
  char *arg_ptr;
  char *arg_end;
  char *curr_arg;
  size_t argmax;

  char *ret;
  int32_t bufsize = 500;

  ret = (char *)calloc(bufsize, sizeof(char));
  check_mem(ret);

  // special case for PID 0 (kernel_task) where cmdline cannot be fetched
  if (pid == 0)
    return 0;

  // read argmax and allocate memory for argument space.
  argmax = get_argmax();
  check(argmax, "");

  procargs = (char *)calloc(1, argmax);
  check_mem(procargs);

  // read argument space
  mib[0] = CTL_KERN;
  mib[1] = KERN_PROCARGS2;
  mib[2] = pid;
  if (sysctl(mib, 3, procargs, &argmax, NULL, 0) < 0) {
    if (EINVAL == errno) {
      // EINVAL == access denied OR nonexistent PID
      if (pid_exists(pid))
        log_err("Access denied");
      else
        log_err("No such process");
    }
    goto error;
  }

  arg_end = &procargs[argmax];
  // copy the number of arguments to nargs
  memcpy(&nargs, procargs, sizeof(nargs));

  arg_ptr = procargs + sizeof(nargs);
  len = strlen(arg_ptr);
  arg_ptr += len + 1;

  if (arg_ptr == arg_end) {
    free(procargs);
    return 0;
  }

  // skip ahead to the first argument
  for (; arg_ptr < arg_end; arg_ptr++) {
    if (*arg_ptr != '\0') {
      break;
    }
  }

  // iterate through arguments
  curr_arg = arg_ptr;
  strcpy(ret, "");
  len = 0;
  while (arg_ptr < arg_end && nargs > 0) {
    if (*arg_ptr++ == '\0') {
      strcat(ret, curr_arg);
      len += strlen(curr_arg);
      ret[len] = ' ';
      ret[++len] = '\0';

      // iterate to next arg and decrement # of args
      curr_arg = arg_ptr;
      nargs--;
    }
  }
  ret[--len] = '\0';

  free(procargs);

  if (len == bufsize) {
    log_warn("TODO: Long command line. Returning only partial string");
    return ret;
  }
  if (len <= bufsize) {
    return ret;
  }

error:
  free(procargs);
  free(ret);
  return NULL;
}

static double get_create_time(pid_t pid) {
  struct kinfo_proc kp;

  if (!get_kinfo_proc(pid, &kp))
    return -1;

  return TV2DOUBLE(kp.kp_proc.p_starttime);
}

static long *get_uids(pid_t pid) {
  long *ret = (long *)calloc(3, sizeof(long));
  struct kinfo_proc kp;

  if (!get_kinfo_proc(pid, &kp))
    return NULL;

  ret[0] = (long)kp.kp_eproc.e_pcred.p_ruid;
  ret[1] = (long)kp.kp_eproc.e_ucred.cr_uid;
  ret[2] = (long)kp.kp_eproc.e_pcred.p_svuid;

  return ret;
}

static long *get_gids(pid_t pid) {
  long *ret = (long *)calloc(3, sizeof(long));
  struct kinfo_proc kp;

  if (!get_kinfo_proc(pid, &kp))
    return NULL;

  ret[0] = (long)kp.kp_eproc.e_pcred.p_rgid;
  ret[1] = (long)kp.kp_eproc.e_ucred.cr_groups[0];
  ret[2] = (long)kp.kp_eproc.e_pcred.p_svgid;

  return ret;
}

static char *get_username(uint32_t ruid) {
  struct passwd *ret = NULL;
  char *username = NULL;
  ret = getpwuid(ruid);
  check(ret, "Couldn't access passwd database for entry %d", ruid);
  username = strdup(ret->pw_name);
  check(username, "Couldn't allocate memory for name");
  return username;
error:
  return NULL;
}

static char *get_terminal(pid_t pid) {
  struct kinfo_proc kp;
  dev_t dev;
  char *ttname;
  char *ret;

  if (!get_kinfo_proc(pid, &kp))
    return NULL;

  dev = kp.kp_eproc.e_tdev;
  if (dev == NODEV || (ttname = devname(dev, S_IFCHR)) == NULL)
    return strdup("??");
  else {
    ret = strdup("/dev/");
    ret = (char *)realloc(ret, strlen(ret) + strlen(ttname));
    strcat(ret, ttname);
    return ret;
  }
}

/* Public functions */

bool disk_usage(const char path[], DiskUsage *ret) {
  struct statvfs s;
  int32_t r;
  r = statvfs(path, &s);
  check(r == 0, "Error in calling statvfs for %s", path);
  ret->free = s.f_bavail * s.f_frsize;
  ret->total = s.f_blocks * s.f_frsize;
  ret->used = (s.f_blocks - s.f_bfree) * s.f_frsize;
  ret->percent = percentage(ret->used, ret->total);

  return true;
error:
  return false;
}

DiskPartitionInfo *disk_partitions(bool physical) {
  int32_t num;
  long len;
  uint64_t flags;
  char opts[400];
  struct statfs *fs = NULL;
  uint32_t nparts = 5;
  DiskPartitionInfo *ret = NULL;

  DiskPartition *partitions =
      (DiskPartition *)calloc(nparts, sizeof(DiskPartition));
  ret = (DiskPartitionInfo *)calloc(1, sizeof(DiskPartitionInfo));
  DiskPartition *d = partitions;
  check_mem(partitions);
  check_mem(ret);

  ret->nitems = 0;
  ret->partitions = partitions;

  // get the number of mount points
  num = getfsstat(NULL, 0, MNT_NOWAIT);
  check(num != -1, "");

  len = sizeof(*fs) * num;
  fs = (struct statfs *)calloc(1, len);
  check_mem(fs);

  num = getfsstat(fs, len, MNT_NOWAIT);
  check(num != -1, "");

  for (int32_t i = 0; i < num; i++) {
    opts[0] = 0;
    flags = fs[i].f_flags;

    // see sys/mount.h
    if (flags & MNT_RDONLY)
      strlcat(opts, "ro", sizeof(opts));
    else
      strlcat(opts, "rw", sizeof(opts));
    if (flags & MNT_SYNCHRONOUS)
      strlcat(opts, ",sync", sizeof(opts));
    if (flags & MNT_NOEXEC)
      strlcat(opts, ",noexec", sizeof(opts));
    if (flags & MNT_NOSUID)
      strlcat(opts, ",nosuid", sizeof(opts));
    if (flags & MNT_UNION)
      strlcat(opts, ",union", sizeof(opts));
    if (flags & MNT_ASYNC)
      strlcat(opts, ",async", sizeof(opts));
    if (flags & MNT_EXPORTED)
      strlcat(opts, ",exported", sizeof(opts));
    if (flags & MNT_QUARANTINE)
      strlcat(opts, ",quarantine", sizeof(opts));
    if (flags & MNT_LOCAL)
      strlcat(opts, ",local", sizeof(opts));
    if (flags & MNT_QUOTA)
      strlcat(opts, ",quota", sizeof(opts));
    if (flags & MNT_ROOTFS)
      strlcat(opts, ",rootfs", sizeof(opts));
    if (flags & MNT_DOVOLFS)
      strlcat(opts, ",dovolfs", sizeof(opts));
    if (flags & MNT_DONTBROWSE)
      strlcat(opts, ",dontbrowse", sizeof(opts));
    if (flags & MNT_IGNORE_OWNERSHIP)
      strlcat(opts, ",ignore-ownership", sizeof(opts));
    if (flags & MNT_AUTOMOUNTED)
      strlcat(opts, ",automounted", sizeof(opts));
    if (flags & MNT_JOURNALED)
      strlcat(opts, ",journaled", sizeof(opts));
    if (flags & MNT_NOUSERXATTR)
      strlcat(opts, ",nouserxattr", sizeof(opts));
    if (flags & MNT_DEFWRITE)
      strlcat(opts, ",defwrite", sizeof(opts));
    if (flags & MNT_MULTILABEL)
      strlcat(opts, ",multilabel", sizeof(opts));
    if (flags & MNT_NOATIME)
      strlcat(opts, ",noatime", sizeof(opts));
    if (flags & MNT_UPDATE)
      strlcat(opts, ",update", sizeof(opts));
    if (flags & MNT_RELOAD)
      strlcat(opts, ",reload", sizeof(opts));
    if (flags & MNT_FORCE)
      strlcat(opts, ",force", sizeof(opts));
    if (flags & MNT_CMDFLAGS)
      strlcat(opts, ",cmdflags", sizeof(opts));

    struct stat tmp_buf;
    if (physical && fs[i].f_mntfromname[0] != '/' &&
        stat(fs[i].f_mntfromname, &tmp_buf) != 0) {
      /* Skip this device since we only need physical devices */
      continue;
    }
    d->device = strdup(fs[i].f_mntfromname);   // device
    d->mountpoint = strdup(fs[i].f_mntonname); // mount point
    d->fstype = strdup(fs[i].f_fstypename);    // fs type
    d->opts = strdup(opts);                    // options

    ret->nitems++;
    d++;

    if (ret->nitems == nparts) {
      nparts *= 2;
      partitions =
          (DiskPartition *)realloc(partitions, sizeof(DiskPartition) * nparts);
      check_mem(partitions);
      ret->partitions = partitions;
      d = ret->partitions + ret->nitems; /* Move the cursor to the correct
                                            value in case the realloc moved
                                            the memory */
    }
  }
  free(fs);
  return ret;

error:
  free(fs);
  free_disk_partition_info(ret);
  return NULL;
}

DiskIOCounterInfo *disk_io_counters() {
  CFDictionaryRef parent_dict;
  CFDictionaryRef props_dict;
  CFDictionaryRef stats_dict;
  io_registry_entry_t parent;
  io_registry_entry_t disk;
  io_iterator_t disk_list;

  DiskIOCounters *counters = NULL;
  DiskIOCounters *disk_info = NULL;
  DiskIOCounterInfo *ret =
      (DiskIOCounterInfo *)calloc(1, sizeof(DiskIOCounterInfo));
  check_mem(ret);

  // Get list of disks
  if (IOServiceGetMatchingServices(kIOMasterPortDefault,
                                   IOServiceMatching(kIOMediaClass),
                                   &disk_list) != kIOReturnSuccess) {
    log_err("unable to get the list of disks.");
    goto error;
  }

  // We don't handle more than 30 partitions
  // TODO: see if sizeof disk_list can be found
  counters = (DiskIOCounters *)calloc(30, sizeof(DiskIOCounters));
  check_mem(counters);

  ret->iocounters = counters;

  disk_info = counters;

  // Iterate over disks
  while ((disk = IOIteratorNext(disk_list)) != 0) {
    parent_dict = NULL;
    props_dict = NULL;
    stats_dict = NULL;

    if (IORegistryEntryGetParentEntry(disk, kIOServicePlane, &parent) !=
        kIOReturnSuccess) {
      log_err("unable to get the disk's parent.");
      IOObjectRelease(disk);
      goto error;
    }

    if (IOObjectConformsTo(parent, "IOBlockStorageDriver")) {
      if (IORegistryEntryCreateCFProperties(
              disk, (CFMutableDictionaryRef *)&parent_dict, kCFAllocatorDefault,
              kNilOptions) != kIOReturnSuccess) {
        log_err("unable to get the parent's properties.");
        IOObjectRelease(disk);
        IOObjectRelease(parent);
        goto error;
      }

      if (IORegistryEntryCreateCFProperties(
              parent, (CFMutableDictionaryRef *)&props_dict,
              kCFAllocatorDefault, kNilOptions) != kIOReturnSuccess) {
        log_err("unable to get the disk properties.");
        CFRelease(props_dict);
        IOObjectRelease(disk);
        IOObjectRelease(parent);
        goto error;
      }

      const int32_t kMaxDiskNameSize = 64;
      CFStringRef disk_name_ref =
          (CFStringRef)CFDictionaryGetValue(parent_dict, CFSTR(kIOBSDNameKey));
      char disk_name[kMaxDiskNameSize];

      CFStringGetCString(disk_name_ref, disk_name, kMaxDiskNameSize,
                         CFStringGetSystemEncoding());

      stats_dict = (CFDictionaryRef)CFDictionaryGetValue(
          props_dict, CFSTR(kIOBlockStorageDriverStatisticsKey));

      check(stats_dict, "Unable to get disk stats.");

      CFNumberRef number;
      int64_t reads = 0;
      int64_t writes = 0;
      int64_t read_bytes = 0;
      int64_t write_bytes = 0;
      int64_t read_time = 0;
      int64_t write_time = 0;

      // Get disk reads/writes
      if ((number = (CFNumberRef)CFDictionaryGetValue(
               stats_dict, CFSTR(kIOBlockStorageDriverStatisticsReadsKey)))) {
        CFNumberGetValue(number, kCFNumberSInt64Type, &reads);
      }
      if ((number = (CFNumberRef)CFDictionaryGetValue(
               stats_dict, CFSTR(kIOBlockStorageDriverStatisticsWritesKey)))) {
        CFNumberGetValue(number, kCFNumberSInt64Type, &writes);
      }

      // Get disk bytes read/written
      if ((number = (CFNumberRef)CFDictionaryGetValue(
               stats_dict,
               CFSTR(kIOBlockStorageDriverStatisticsBytesReadKey)))) {
        CFNumberGetValue(number, kCFNumberSInt64Type, &read_bytes);
      }
      if ((number = (CFNumberRef)CFDictionaryGetValue(
               stats_dict,
               CFSTR(kIOBlockStorageDriverStatisticsBytesWrittenKey)))) {
        CFNumberGetValue(number, kCFNumberSInt64Type, &write_bytes);
      }

      // Get disk time spent reading/writing (nanoseconds)
      if ((number = (CFNumberRef)CFDictionaryGetValue(
               stats_dict,
               CFSTR(kIOBlockStorageDriverStatisticsTotalReadTimeKey)))) {
        CFNumberGetValue(number, kCFNumberSInt64Type, &read_time);
      }
      if ((number = (CFNumberRef)CFDictionaryGetValue(
               stats_dict,
               CFSTR(kIOBlockStorageDriverStatisticsTotalWriteTimeKey)))) {
        CFNumberGetValue(number, kCFNumberSInt64Type, &write_time);
      }

      disk_info->name = strdup(disk_name);
      disk_info->reads = reads;
      disk_info->writes = writes;
      disk_info->readbytes = read_bytes;
      disk_info->writebytes = write_bytes;
      // Read/Write time on OS X comes back in nanoseconds, convert to
      // milliseconds as in psutil
      disk_info->readtime = read_time / 1000 / 1000;
      disk_info->writetime = write_time / 1000 / 1000;

      disk_info++;
      ret->nitems++;

      CFRelease(parent_dict);
      IOObjectRelease(parent);
      CFRelease(props_dict);
      IOObjectRelease(disk);
    }
  }

  IOObjectRelease(disk_list);

  return ret;

error:
  free_disk_iocounter_info(ret);
  return NULL;
}

NetIOCounterInfo *net_io_counters() {
  char *buf = NULL, *lim, *next;
  struct if_msghdr *ifm;
  int32_t mib[6];
  size_t len;
  int32_t ninterfaces = 0;
  NetIOCounterInfo *ret = NULL;
  NetIOCounters *counters = NULL;
  NetIOCounters *nc = NULL;
  ret = (NetIOCounterInfo *)calloc(1, sizeof(NetIOCounterInfo));
  counters = (NetIOCounters *)calloc(15, sizeof(NetIOCounters));

  check_mem(ret);
  check_mem(counters);
  nc = counters;

  mib[0] = CTL_NET;        // networking subsystem
  mib[1] = PF_ROUTE;       // type of information
  mib[2] = 0;              // protocol (IPPROTO_xxx)
  mib[3] = 0;              // address family
  mib[4] = NET_RT_IFLIST2; // operation
  mib[5] = 0;

  check(!(sysctl(mib, 6, NULL, &len, NULL, 0) < 0), "");

  buf = (char *)calloc(1, len);
  check_mem(buf);

  check(!(sysctl(mib, 6, buf, &len, NULL, 0) < 0), "");

  lim = buf + len;

  for (next = buf; next < lim;) {
    ifm = (struct if_msghdr *)next;
    next += ifm->ifm_msglen;

    if (ifm->ifm_type == RTM_IFINFO2) {
      ninterfaces++;

      struct if_msghdr2 *if2m = (struct if_msghdr2 *)ifm;
      struct sockaddr_dl *sdl = (struct sockaddr_dl *)(if2m + 1);
      char ifc_name[32];

      strncpy(ifc_name, sdl->sdl_data, sdl->sdl_nlen);
      ifc_name[sdl->sdl_nlen] = 0;

      nc->name = strdup(ifc_name);
      nc->bytes_sent = if2m->ifm_data.ifi_obytes;
      nc->bytes_recv = if2m->ifm_data.ifi_ibytes;
      nc->packets_sent = if2m->ifm_data.ifi_opackets;
      nc->packets_recv = if2m->ifm_data.ifi_ipackets;
      nc->errin = if2m->ifm_data.ifi_ierrors;
      nc->errout = if2m->ifm_data.ifi_oerrors;
      nc->dropin = if2m->ifm_data.ifi_iqdrops;
      nc->dropout = 0; // dropout not supported

      nc++;

    } else {
      continue;
    }
  }

  free(buf);
  ret->iocounters = counters;
  ret->nitems = ninterfaces;
  return ret;

error:
  free(buf);
  free(counters);
  free(nc);
  return NULL;
}

uint32_t get_boot_time() {
  /* read KERN_BOOTIME */
  int32_t mib[2] = {CTL_KERN, KERN_BOOTTIME};
  struct timeval result;
  size_t len = sizeof result;
  time_t boot_time = 0;

  check(sysctl(mib, 2, &result, &len, NULL, 0) != -1, "sysctl failed");
  boot_time = result.tv_sec;
  // For some odd reason, test fails without this type cast
  return (float)boot_time;

error:
  return -1;
}

CpuTimes *cpu_times(bool percpu) {
  CpuTimes *ret = NULL;

  if (!percpu) {
    ret = (CpuTimes *)calloc(1, sizeof(CpuTimes));

    mach_msg_type_number_t count = HOST_CPU_LOAD_INFO_COUNT;
    kern_return_t kerror;
    host_cpu_load_info_data_t r_load;

    mach_port_t host_port = mach_host_self();
    kerror = host_statistics(host_port, HOST_CPU_LOAD_INFO,
                             (host_info_t)&r_load, &count);
    check(kerror == KERN_SUCCESS, "Error in host_statistics(): %s",
          mach_error_string(kerror));
    mach_port_deallocate(mach_task_self(), host_port);

    ret->user = (double)r_load.cpu_ticks[CPU_STATE_USER] / CLK_TCK;
    ret->nice = (double)r_load.cpu_ticks[CPU_STATE_NICE] / CLK_TCK;
    ret->system = (double)r_load.cpu_ticks[CPU_STATE_SYSTEM] / CLK_TCK;
    ret->idle = (double)r_load.cpu_ticks[CPU_STATE_IDLE] / CLK_TCK;

    return ret;
  } else {
    return per_cpu_times();
  }

error:
  free(ret);
  return NULL;
}

double *cpu_util_percent(bool percpu, CpuTimes *prev_times) {
  CpuTimes *current = NULL;
  uint32_t ncpus = percpu ? cpu_count(true) : 1;
  double *percentage = (double *)calloc(ncpus, sizeof(double));
  check(prev_times, "Need a reference point. prev_times can't be NULL");

  current = cpu_times(percpu);
  check(current, "Couldn't obtain CPU times");

  for (uint32_t i = 0; i < ncpus; i++) {
    percentage[i] = calculate_cpu_util_percentage(prev_times + i, current + i);
  }
  free(current);
  return percentage;
error:
  free(current);
  return NULL;
}

CpuTimes *cpu_times_percent(bool percpu, CpuTimes *prev_times) {
  CpuTimes *current = NULL;
  CpuTimes *t;
  uint32_t ncpus = percpu ? cpu_count(true) : 1;
  CpuTimes *ret;
  check(prev_times, "Need a reference point. prev_times can't be NULL");
  current = cpu_times(percpu);
  check(current, "Couldn't obtain CPU times");
  ret = (CpuTimes *)calloc(ncpus, sizeof(CpuTimes));
  check_mem(ret);
  for (uint32_t i = 0; i < ncpus; i++) {
    t = calculate_cpu_times_percentage(prev_times + i, current + i);
    *(ret + i) = *t;
    free(t);
  }
  free(current);
  return ret;
error:
  free(current);
  return NULL;
}

uint32_t cpu_count(bool logical) {
  uint32_t ncpu;
  size_t len = sizeof(ncpu);

  if (logical) {
    check(sysctlbyname("hw.logicalcpu", &ncpu, &len, NULL, 0) != -1,
          "sysctl failed");
  } else {
    check(sysctlbyname("hw.physicalcpu", &ncpu, &len, NULL, 0) != -1,
          "sysctl failed");
  }

  return ncpu;

error:
  return -1;
}

UsersInfo *get_users() {
  uint32_t nusers = 100;

  UsersInfo *ret = (UsersInfo *)calloc(1, sizeof(UsersInfo));
  check_mem(ret);
  Users *users = (Users *)calloc(nusers, sizeof(Users));
  check_mem(users);
  Users *u = users;

  struct utmpx *utx;

  ret->nitems = 0;
  ret->users = users;

  while (NULL != (utx = getutxent())) {
    if (utx->ut_type != USER_PROCESS)
      continue;

    u->username = strdup(utx->ut_user);
    check_mem(u->username);

    u->tty = strdup(utx->ut_line);
    check_mem(u->tty);

    u->hostname = strdup(utx->ut_host);
    check_mem(u->hostname);

    u->tstamp = utx->ut_tv.tv_sec;

    ret->nitems++;
    u++;

    if (ret->nitems ==
        nusers) { /* More users than we've allocated space for. */
      nusers *= 2;
      users = realloc(users, sizeof(Users) * nusers);
      check_mem(users);
      ret->users = users;
      u = ret->users + ret->nitems; /* Move the cursor to the correct
                                       value in case the realloc moved
                                       the memory */
    }
  }
  endutxent();
  return ret;

error:
  free_users_info(ret);
  return NULL;
}

/* Check whether pid exists in the current process table. */
bool pid_exists(pid_t pid) {
  if (pid == 0) // see `man 2 kill` for pid zero
    return true;

  if (kill(pid, 0) == -1) {
    if (errno == ESRCH) {
      log_err("No such process");
      return false;
    } else if (errno == EPERM) {
      // permission denied, but process does exist
      return true;
    } else {
      // log error with errno
      log_err("");
      return false;
    }
  }
  return true;
}

Process *get_process(pid_t pid) {
  /* TODO: Add test for invalid pid. Right now, we get a lot of errors and some
   * structure.*/
  Process *retval = (Process *)calloc(1, sizeof(Process));
  long *uids = NULL;
  long *gids = NULL;
  retval->pid = pid;
  retval->ppid = get_ppid(pid);
  retval->name = get_procname(pid);
  retval->exe = get_exe(pid);
  retval->cmdline = get_cmdline(pid);
  retval->create_time = get_create_time(pid);
  uids = get_uids(pid);
  if (uids) {
    retval->uid = uids[0];
    retval->euid = uids[1];
    retval->suid = uids[2];
    retval->username =
        get_username(retval->uid); /* Uses real uid and not euid */
  } else {
    retval->uid = retval->euid = retval->suid = 0;
    retval->username = NULL;
  }

  gids = get_gids(pid);
  if (uids) {
    retval->gid = gids[0];
    retval->egid = gids[1];
    retval->sgid = gids[2];
  } else {
    retval->uid = retval->euid = retval->suid = 0;
  }

  retval->terminal = get_terminal(pid);
  free(uids);
  free(gids);
  return retval;
}

bool swap_memory(SwapMemInfo *ret) {
  int32_t mib[2];
  size_t size;
  struct xsw_usage totals;
  vm_statistics_data_t vmstat;
  int32_t pagesize = getpagesize();

  mib[0] = CTL_VM;
  mib[1] = VM_SWAPUSAGE;
  size = sizeof(totals);
  if (sysctl(mib, 2, &totals, &size, NULL, 0) == -1) {
    log_err("sysctl(VM_SWAPUSAGE) failed");
  }
  if (!sys_vminfo(&vmstat)) {
    ret = NULL;
    return false;
  }

  ret->total = totals.xsu_total;
  ret->used = totals.xsu_used;
  ret->free = totals.xsu_avail;
  ret->percent = percentage(totals.xsu_used, totals.xsu_total);
  ret->sin = (unsigned long long)vmstat.pageins * pagesize;
  ret->sout = (unsigned long long)vmstat.pageouts * pagesize;

  return true;
}

bool virtual_memory(VmemInfo *ret) {
  int32_t mib[2];
  uint64_t total;
  size_t len = sizeof(total);
  vm_statistics_data_t vm;
  int32_t pagesize = getpagesize();
  // physical mem
  mib[0] = CTL_HW;
  mib[1] = HW_MEMSIZE;

  // This is also available as sysctlbyname("hw.memsize").
  if (sysctl(mib, 2, &total, &len, NULL, 0)) {
    log_err("sysctl(HW_MEMSIZE) failed");
  }

  // vm
  if (!sys_vminfo(&vm)) {
    ret = NULL;
    return false;
  }

  ret->total = total;
  ret->free = (unsigned long long)vm.free_count * pagesize;
  ret->active = (unsigned long long)vm.active_count * pagesize;
  ret->inactive = (unsigned long long)vm.inactive_count * pagesize;
  ret->wired = (unsigned long long)vm.wire_count * pagesize;
  ret->available = ret->free + ret->inactive;
  ret->percent = percentage((ret->total - ret->available), ret->total);
  ret->used = ret->active + ret->inactive + ret->wired;

  return true;
}

/* Auxiliary public functions for garbage collection */

void free_disk_partition_info(DiskPartitionInfo *di) {
  DiskPartition *d = di->partitions;
  while (di->nitems--) {
    free(d->device);
    free(d->mountpoint);
    free(d->fstype);
    free(d->opts);
    d++;
  }
  free(di->partitions);
  free(di);
}

void free_disk_iocounter_info(DiskIOCounterInfo *di) {
  DiskIOCounters *d = di->iocounters;
  while (di->nitems--) {
    free(d->name);
    d++;
  }
  free(di->iocounters);
  free(di);
}

void free_net_iocounter_info(NetIOCounterInfo *d) {
  uint32_t i;
  for (i = 0; i < d->nitems; i++) {
    free(d->iocounters[i].name);
  }
  free(d->iocounters);
  free(d);
}

void free_users_info(UsersInfo *ui) {
  Users *u = ui->users;
  while (ui->nitems--) {
    free(u->username);
    free(u->tty);
    free(u->hostname);
    u++;
  }
  free(ui->users);
  free(ui);
}

void free_process(Process *p) {
  free(p->name);
  free(p->exe);
  free(p->cmdline);
  free(p->username);
  free(p->terminal);
  free(p);
}

/* Coverage related cruft */

void __gcov_flush(void);
/*
  The following function is an ugly workaround to ensure that coverage
  data can be manually flushed to disk during py.test invocations. If
  this is not done, we cannot measure the coverage information. More details
  in http://... link_to_bug...
 */
void gcov_flush(void) { __gcov_flush(); }
