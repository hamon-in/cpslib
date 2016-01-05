#include <stdio.h>
#include <stdlib.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/sysctl.h>
#include <sys/types.h>
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

/* Internal functions */

static CpuTimes *per_cpu_times() {
  CpuTimes *ret = NULL;

  natural_t cpu_count;
  processor_info_array_t info_array;
  mach_msg_type_number_t info_count;
  kern_return_t kerror;
  processor_cpu_load_info_data_t *cpu_load_info = NULL;
  int sysret;

  mach_port_t host_port = mach_host_self();
  kerror = host_processor_info(host_port, PROCESSOR_CPU_LOAD_INFO, &cpu_count,
                               &info_array, &info_count);
  check(kerror == KERN_SUCCESS, "Error in host_processor_info(): %s",
        mach_error_string(kerror));
  mach_port_deallocate(mach_task_self(), host_port);

  cpu_load_info = (processor_cpu_load_info_data_t *)info_array;

  ret = (CpuTimes *)calloc(cpu_count, sizeof(CpuTimes));
  check_mem(ret);

  for (unsigned int i = 0; i < cpu_count; i++) {
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
                         info_count * sizeof(int));
  if (sysret != KERN_SUCCESS)
    log_warn("vm_deallocate() failed");

  return ret;

error:
  if (ret)
    free(ret);
  if (cpu_load_info != NULL) {
    sysret = vm_deallocate(mach_task_self(), (vm_address_t)info_array,
                           info_count * sizeof(int));
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
  if (ret)
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
static int pslib_sys_vminfo(vm_statistics_data_t *vmstat) {
  kern_return_t ret;
  mach_msg_type_number_t count = sizeof(*vmstat) / sizeof(integer_t);
  mach_port_t mport = mach_host_self();

  ret = host_statistics(mport, HOST_VM_INFO, (host_info_t)vmstat, &count);
  if (ret != KERN_SUCCESS) {
    log_err("host_statistics() failed: %s", mach_error_string(ret));
    return 0;
  }
  mach_port_deallocate(mach_task_self(), mport);
  return 1;
}

/* Public functions */

int disk_usage(char path[], DiskUsage *ret) {
  struct statvfs s;
  int r;
  r = statvfs(path, &s);
  check(r == 0, "Error in calling statvfs for %s", path);
  ret->free = s.f_bavail * s.f_frsize;
  ret->total = s.f_blocks * s.f_frsize;
  ret->used = (s.f_blocks - s.f_bfree) * s.f_frsize;
  ret->percent = percentage(ret->used, ret->total);

  return 0;
error:
  return -1;
}

DiskPartitionInfo *disk_partitions(int physical) {
  int num;
  int i;
  long len;
  uint64_t flags;
  char opts[400];
  struct statfs *fs = NULL;
  int nparts = 5;

  DiskPartition *partitions =
      (DiskPartition *)calloc(nparts, sizeof(DiskPartition));
  DiskPartitionInfo *ret =
      (DiskPartitionInfo *)calloc(1, sizeof(DiskPartitionInfo));
  DiskPartition *d = partitions;
  check_mem(partitions);
  check_mem(ret);

  ret->nitems = 0;
  ret->partitions = partitions;

  // get the number of mount points
  num = getfsstat(NULL, 0, MNT_NOWAIT);
  check(num != -1, "");

  len = sizeof(*fs) * num;
  fs = malloc(len);
  check_mem(fs);

  num = getfsstat(fs, len, MNT_NOWAIT);
  check(num != -1, "");

  for (i = 0; i < num; i++) {
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
  if (fs != NULL)
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

      const int kMaxDiskNameSize = 64;
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
  int mib[6];
  size_t len;
  int ninterfaces = 0;

  NetIOCounterInfo *ret =
      (NetIOCounterInfo *)calloc(1, sizeof(NetIOCounterInfo));
  NetIOCounters *counters = (NetIOCounters *)calloc(15, sizeof(NetIOCounters));
  NetIOCounters *nc = counters;

  check_mem(ret);
  check_mem(counters);

  mib[0] = CTL_NET;        // networking subsystem
  mib[1] = PF_ROUTE;       // type of information
  mib[2] = 0;              // protocol (IPPROTO_xxx)
  mib[3] = 0;              // address family
  mib[4] = NET_RT_IFLIST2; // operation
  mib[5] = 0;

  check(!(sysctl(mib, 6, NULL, &len, NULL, 0) < 0), "");

  buf = malloc(len);
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
  if (buf != NULL)
    free(buf);
  if (counters)
    free(counters);
  if (nc)
    free(nc);
  return NULL;
}

float get_boot_time() {
  /* read KERN_BOOTIME */
  int mib[2] = {CTL_KERN, KERN_BOOTTIME};
  struct timeval result;
  size_t len = sizeof result;
  time_t boot_time = 0;

  check(sysctl(mib, 2, &result, &len, NULL, 0) != -1, "sysctl failed");
  boot_time = result.tv_sec;
  return (float)boot_time;

error:
  return -1;
}

CpuTimes *cpu_times(int percpu) {
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
  if (ret)
    free(ret);
  return NULL;
}

double *cpu_util_percent(int percpu, CpuTimes *prev_times) {
  CpuTimes *current = NULL;
  int i, ncpus = percpu ? cpu_count(1) : 1;
  double *percentage = (double *)calloc(ncpus, sizeof(double));
  check(prev_times, "Need a reference point. prev_times can't be NULL");

  current = cpu_times(percpu);
  check(current, "Couldn't obtain CPU times");

  for (i = 0; i < ncpus; i++) {
    percentage[i] = calculate_cpu_util_percentage(prev_times + i, current + i);
  }
  free(current);
  return percentage;
error:
  if (current)
    free(current);
  return NULL;
}

CpuTimes *cpu_times_percent(int percpu, CpuTimes *prev_times) {
  CpuTimes *current = NULL;
  CpuTimes *t;
  int i, ncpus = percpu ? cpu_count(1) : 1;
  CpuTimes *ret;
  check(prev_times, "Need a reference point. prev_times can't be NULL");
  current = cpu_times(percpu);
  check(current, "Couldn't obtain CPU times");
  ret = (CpuTimes *)calloc(ncpus, sizeof(CpuTimes));
  check_mem(ret);
  for (i = 0; i < ncpus; i++) {
    t = calculate_cpu_times_percentage(prev_times + i, current + i);
    *(ret + i) = *t;
    free(t);
  }
  free(current);
  return ret;
error:
  if (current)
    free(current);
  return NULL;
}

int cpu_count(int logical) {
  int ncpu;
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
  int nusers = 100;

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

Process *get_process(pid_t pid) {
  pid = 0;
  return NULL;
}

int swap_memory(SwapMemInfo *ret) {
  int mib[2];
  size_t size;
  struct xsw_usage totals;
  vm_statistics_data_t vmstat;
  int pagesize = getpagesize();

  mib[0] = CTL_VM;
  mib[1] = VM_SWAPUSAGE;
  size = sizeof(totals);
  if (sysctl(mib, 2, &totals, &size, NULL, 0) == -1) {
    log_err("sysctl(VM_SWAPUSAGE) failed");
  }
  if (!pslib_sys_vminfo(&vmstat)) {
    ret = NULL;
    return -1;
  }

  ret->total = totals.xsu_total;
  ret->used = totals.xsu_used;
  ret->free = totals.xsu_avail;
  ret->percent = percentage(totals.xsu_used, totals.xsu_total);
  ret->sin = (unsigned long long)vmstat.pageins * pagesize;
  ret->sout = (unsigned long long)vmstat.pageouts * pagesize;

  return 0;
}

int virtual_memory(VmemInfo *ret) {
  ret = NULL;
  return 0;
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
  int i;
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
