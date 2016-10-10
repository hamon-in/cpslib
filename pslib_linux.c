#include <ctype.h>
#include <limits.h>
#include <mntent.h>
#include <pwd.h>
#include <search.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/sysinfo.h>
#include <unistd.h>
#include <utmp.h>

#include "pslib.h"
#include "common.h"

void __gcov_flush(void);

static int clean_cmdline(char *ip, int len)
/* Replaces all '\0' with ' ' in the string ip (bounded by length len).
   and then adds a '\0' at the end. This is used to parse the cmdline file
   inside proc where parts of the command line are separated using '\0'.

   Returns number of replacements
*/
{
  int i = 0;
  int replacements = 0;
  for (i = 0; i < len; i++) {
    if (ip[i] == '\0') {
      ip[i] = ' ';
      replacements++;
    }
  }
  ip[--i] = '\0';
  return replacements;
}

/* Internal functions */

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

static int logical_cpu_count() {
  int ret;
  ret = (int)sysconf(_SC_NPROCESSORS_ONLN);
  if (ret == -1) {
    /* TDB: Parse /proc/cpuinfo */
    ;
  }
  return ret;
}

static int physical_cpu_count() {
  FILE *fp = NULL;
  size_t s = 100;
  long int ids[s]; /* Assume we don't have more than 100 physical CPUs */
  memset(&ids, -1, sizeof(long int) * 100);
  long int *cid = ids;
  long int id;
  int nprocs = 0;
  char *line = (char *)calloc(100, sizeof(char));
  fp = fopen("/proc/cpuinfo", "r");
  check(fp, "Couldn't open '/proc/cpuinfo'");
  check_mem(line);

  while (fgets(line, 90, fp) != NULL) {
    if (strncasecmp(line, "physical id", 11) == 0) {
      strtok(line, ":");
      id = strtol(strtok(NULL, " "), NULL,
                  10); /* TODO: Assuming that physical id is a number */
      if (!lfind(&id, ids, &s, sizeof(int),
                 int_comp)) { /* TODO: Replace this with lsearch */
        *cid = id;
        id++;
        nprocs += 1;
      } else {
        ;
      }
    }
  }

  fclose(fp);
  free(line);
  return nprocs;
error:
  if (fp)
    fclose(fp);
  free(line);
  return -1;
}

char **get_physical_devices(size_t *ndevices) {
  FILE *fs = NULL;
  char *line = NULL;
  char **retval = NULL;
  unsigned int i;
  *ndevices = 0;

  line = (char *)calloc(60, sizeof(char));
  check_mem(line);

  retval = (char **)calloc(100, sizeof(char *));
  check_mem(retval);

  fs = fopen("/proc/filesystems", "r");
  check(fs, "Couldn't open /proc/filesystems");

  while (fgets(line, 50, fs) != NULL) {
    if (strncmp(line, "nodev", 5) != 0) {
      line = squeeze(line, " \t\n");
      retval[*ndevices] = strdup(line);
      *ndevices += 1;
      check(*ndevices < 100,
            "FIXME: Can't process more than 100 physical devices");
    }
  }
  fclose(fs);
  free(line);
  return retval;
error:
  if (fs)
    fclose(fs);
  free(line);
  if (*ndevices != 0)
    for (i = 0; i < *ndevices; i++)
      free(retval[i]);
  return NULL;
}

static pid_t get_ppid(pid_t pid) {
  FILE *fp = NULL;
  pid_t ppid = -1;
  char *tmp;
  char procfile[50];

  sprintf(procfile, "/proc/%d/status", pid);
  fp = fopen(procfile, "r");
  check(fp, "Couldn't open process status file");
  tmp = grep_awk(fp, "PPid", 1, ":");
  ppid = tmp ? atoi(tmp) : -1;

  check(ppid != -1, "Couldnt' find PPid in process status file");
  fclose(fp);
  free(tmp);

  return ppid;
error:
  if (fp)
    fclose(fp);
  return -1;
}

static char *get_procname(pid_t pid) {
  FILE *fp = NULL;
  char *tmp;
  char procfile[50];
  char line[350];

  sprintf(procfile, "/proc/%d/stat", pid);
  fp = fopen(procfile, "r");
  check(fp, "Couldn't open process status file");
  check(fgets(line, 300, fp), "Couldn't read from process status file");
  fclose(fp);

  tmp = strtok(line, " ");
  tmp = strtok(NULL, " "); /* Name field */
  tmp = squeeze(tmp, "()");

  return strdup(tmp);
error:
  if (fp)
    fclose(fp);
  return NULL;
}

static char *get_exe(pid_t pid) {
  FILE *fp = NULL;
  char *tmp = NULL;
  char procfile[50];
  ssize_t ret;
  unsigned int bufsize = 1024;
  struct stat buf;

  sprintf(procfile, "/proc/%d/exe", pid);
  tmp = (char *)calloc(bufsize, sizeof(char));
  check_mem(tmp);
  ret = readlink(procfile, tmp, bufsize - 1);
  if (ret == -1 && errno == ENOENT) {
    if (lstat(procfile, &buf) == 0) {
      debug("Probably a system process. No executable");
      strcpy(tmp, "");
      return tmp;
    } else {
      sentinel("No such process");
    }
  }
  check(ret != -1, "Couldn't expand symbolic link");
  while ((size_t) ret == bufsize - 1) {
    /* Buffer filled. Might be incomplete. Increase size and try again. */
    bufsize *= 2;
    tmp = (char *)realloc(tmp, bufsize);
    ret = readlink(procfile, tmp, bufsize - 1);
    check(ret != -1, "Couldn't expand symbolic link");
  }
  tmp[ret] = '\0';
  return tmp;
error:
  if (fp)
    fclose(fp);
  free(tmp);
  return NULL;
}

static char *get_cmdline(pid_t pid) {
  FILE *fp = NULL;
  char procfile[50];
  char *contents = NULL;
  int bufsize = 500;
  ssize_t read;

  contents = (char *)calloc(bufsize, sizeof(char));
  check_mem(contents);
  sprintf(procfile, "/proc/%d/cmdline", pid);
  fp = fopen(procfile, "r");
  check(fp, "Couldn't open process cmdline file");
  read = fread(contents, sizeof(char), bufsize, fp);
  clean_cmdline(contents, read);
  fclose(fp);
  if (read == bufsize) {
    log_warn("TODO: Long command line. Returning only partial string");
    return contents;
  }
  if (read <= bufsize) {
    return contents;
  }
error:
  if (fp)
    fclose(fp);
  free(contents);
  return NULL;
}

static double get_create_time(pid_t pid) {
  FILE *fp = NULL;
  char procfile[50];
  char s_pid[10];
  char *tmp;
  double ct_jiffies;
  double ct_seconds;
  double clock_ticks = sysconf(_SC_CLK_TCK);
  long boot_time = get_boot_time();

  sprintf(s_pid, "%d", pid);
  sprintf(procfile, "/proc/%d/stat", pid);
  fp = fopen(procfile, "r");
  check(fp, "Couldn't open process stat file");
  tmp = grep_awk(fp, s_pid, 21, " ");
  ct_jiffies = atof(tmp);
  free(tmp);
  fclose(fp);
  ct_seconds = boot_time + (ct_jiffies / clock_ticks);
  return ct_seconds;
error:
  if (fp)
    fclose(fp);
  return -1;
}

static unsigned int *get_ids(pid_t pid, const char *field)
/* field parameter is used to determine which line to parse (Uid or Gid) */
{
  FILE *fp = NULL;
  char *tmp;
  char procfile[50];
  char line[400];
  unsigned int *retval = NULL;

  sprintf(procfile, "/proc/%d/status", pid);
  fp = fopen(procfile, "r");
  check(fp, "Couldn't open process status file");
  while (fgets(line, 399, fp) != NULL) {
    if (strncmp(line, field, 4) == 0) {
      retval = (unsigned int *)calloc(3, sizeof(unsigned int));
      check_mem(retval);
      tmp = strtok(line, "\t");
      tmp = strtok(NULL, "\t");
      retval[0] = strtoul(tmp, NULL, 10); /* Real UID */
      tmp = strtok(NULL, "\t");
      retval[1] = strtoul(tmp, NULL, 10); /* Effective UID */
      tmp = strtok(NULL, "\t");
      retval[2] = strtoul(tmp, NULL, 10); /* Saved UID */
      break;
    }
  }

  check(retval != NULL, "Couldnt' find Uid in process status file");
  fclose(fp);

  return retval;
error:
  if (fp)
    fclose(fp);
  return NULL;
}

static char *get_username(unsigned int ruid) {
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
  FILE *fp = NULL;
  char *tmp = NULL;
  char procfile[50];
  ssize_t ret;
  unsigned int bufsize = 1024;

  sprintf(procfile, "/proc/%d/fd/0", pid);
  tmp = (char *)calloc(bufsize, sizeof(char));
  check_mem(tmp);
  ret = readlink(procfile, tmp, bufsize - 1);
  check(ret != -1, "Couldn't expand symbolic link");
  while ((size_t) ret == bufsize - 1) {
    /* Buffer filled. Might be incomplete. Increase size and try again. */
    bufsize *= 2;
    tmp = (char *)realloc(tmp, bufsize);
    ret = readlink(procfile, tmp, bufsize - 1);
    check(ret != -1, "Couldn't expand symbolic link");
  }
  tmp[ret] = '\0';
  return tmp;
error:
  if (fp)
    fclose(fp);
  free(tmp);
  return NULL;
}

int parse_cpu_times(char *line, CpuTimes *ret) {
  unsigned long values[10] = {};
  double clock_ticks;
  int i;
  char *pos = strtok(line, " ");

  pos = strtok(NULL, " "); // skip cpu
  for (i = 0; i < 10 && pos != NULL; i++) {
    values[i] = strtoul(pos, NULL, 10);
    pos = strtok(NULL, " ");
  }
  check(i == 10, "Couldn't properly parse cpu times")

      clock_ticks = sysconf(_SC_CLK_TCK);

  ret->user = values[0] / clock_ticks;
  ret->nice = values[1] / clock_ticks;
  ret->system = values[2] / clock_ticks;
  ret->idle = values[3] / clock_ticks;
  ret->iowait = values[4] / clock_ticks;
  ret->irq = values[5] / clock_ticks;
  ret->softirq = values[6] / clock_ticks;
  ret->steal = values[7] / clock_ticks;
  ret->guest = values[8] / clock_ticks;
  ret->guest_nice = values[9] / clock_ticks;

  return 0;
error:
  return -1;
}

/* Public functions */
bool disk_usage(const char path[], DiskUsage *ret) {
  struct statvfs s;
  int r;
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
  FILE *file = NULL;
  struct mntent *entry;
  uint32_t nparts = 5;
  char **phys_devices = NULL;
  size_t nphys_devices;
  unsigned int i;

  DiskPartition *partitions =
      (DiskPartition *)calloc(nparts, sizeof(DiskPartition));
  DiskPartitionInfo *ret =
      (DiskPartitionInfo *)calloc(1, sizeof(DiskPartitionInfo));
  DiskPartition *d = partitions;
  check_mem(partitions);
  check_mem(ret);

  ret->nitems = 0;
  ret->partitions = partitions;

  file = setmntent(MOUNTED, "r");
  check(file, "Couldn't open %s", MOUNTED);

  phys_devices = get_physical_devices(&nphys_devices);

  while ((entry = getmntent(file))) {
    if (physical &&
        !lfind(&entry->mnt_type, phys_devices, &nphys_devices, sizeof(char *),
               str_comp)) {
      /* Skip this partition since we only need physical partitions */
      continue;
    }
    d->device = strdup(entry->mnt_fsname);
    d->mountpoint = strdup(entry->mnt_dir);
    d->fstype = strdup(entry->mnt_type);
    d->opts = strdup(entry->mnt_opts);

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
  endmntent(file);

  for (i = 0; i < nphys_devices; i++) {
    free(phys_devices[i]);
  }
  free(phys_devices);

  return ret;

error:
  if (file)
    endmntent(file);
  free_disk_partition_info(ret);
  return NULL;
}

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

DiskIOCounterInfo *disk_io_counters() {
  const int sector_size = 512;
  char *line = NULL;
  char **partitions = NULL;
  char *tmp;
  int i = 0, nparts = 0;
  size_t nmemb;
  DiskIOCounters *counters = NULL;
  DiskIOCounters *ci = NULL;
  DiskIOCounterInfo *ret = NULL;

  line = (char *)calloc(150, sizeof(char));
  partitions = (char **)calloc(30, sizeof(char *));
  ret = (DiskIOCounterInfo *)calloc(1, sizeof(DiskIOCounterInfo));

  FILE *fp = fopen("/proc/partitions", "r");
  check(fp, "Couldn't open /proc/partitions");

  check_mem(line);
  check_mem(partitions);
  check_mem(ret);
  while (fgets(line, 40, fp) != NULL) {
    if (i++ < 2) {
      continue;
    }
    tmp = strtok(line, " \n"); /* major number */
    tmp = strtok(NULL, " \n"); /* minor number */
    tmp = strtok(NULL, " \n"); /* Number of blocks */
    tmp = strtok(NULL, " \n"); /* Name */
    if (isdigit(tmp[strlen(tmp) -
                    1])) { /* Only partitions (i.e. skip things like 'sda')*/
                           /* TODO: Skipping handling
                            * http://code.google.com/p/psutil/issues/detail?id=338 for now */
      partitions[nparts] = strndup(tmp, 10);
      check_mem(partitions[nparts]);
      nparts++; /* TODO: We can't handle more than 30 partitions now */
    }
  }
  fclose(fp);
  fp = NULL;

  nmemb = nparts;
  counters = (DiskIOCounters *)calloc(nparts, sizeof(DiskIOCounters));
  check_mem(counters);
  ci = counters;
  fp = fopen("/proc/diskstats", "r");
  check(fp, "Couldn't open /proc/diskstats");

  ret->iocounters = counters;

  while (fgets(line, 120, fp) != NULL) {
    tmp = strtok(line, " \n"); /* major number (skip) */
    tmp = strtok(NULL, " \n"); /* minor number (skip) */
    tmp = strtok(NULL, " \n"); /* name */
    if (lfind(&tmp, partitions, &nmemb, sizeof(char *), str_comp)) {
      ci->name = strdup(tmp);
      check_mem(ci->name);

      tmp = strtok(NULL, " \n"); /* reads completed successfully */
      ci->reads = strtoul(tmp, NULL, 10);

      tmp = strtok(NULL, " \n"); /* reads merged (skip)*/
      tmp = strtok(NULL, " \n"); /* sectors read  */
      ci->readbytes = strtoul(tmp, NULL, 10) * sector_size;

      tmp = strtok(NULL, " \n"); /* time spent reading  */
      ci->readtime = strtoul(tmp, NULL, 10);

      tmp = strtok(NULL, " \n"); /* writes completed */
      ci->writes = strtoul(tmp, NULL, 10);

      tmp = strtok(NULL, " \n"); /* writes merged (skip)*/
      tmp = strtok(NULL, " \n"); /* sectors written */
      ci->writebytes = strtoul(tmp, NULL, 10) * sector_size;

      tmp = strtok(NULL, " \n"); /* time spent writing */
      ci->writetime = strtoul(tmp, NULL, 10);

      ci++; /* TODO: Realloc here if necessary */
      ret->nitems++;
    }
  }

  for (i = 0; i < nparts; i++) {
    free(partitions[i]);
  }
  free(partitions);
  free(line);
  fclose(fp);

  return ret;

error:
  if (fp)
    fclose(fp);
  free(line);
  if (partitions) {
    for (i = 0; i < nparts; i++) {
      free(partitions[i]);
    }
    free(partitions);
  }
  if(ret)
    free_disk_iocounter_info(ret);

  return NULL;
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

NetIOCounterInfo *net_io_counters() {
  FILE *fp = NULL;
  NetIOCounterInfo *ret = NULL;
  NetIOCounters *counters = NULL;
  NetIOCounters *nc = NULL;
  int i = 0, ninterfaces = 0;
  char *line = NULL;
  char *tmp = NULL;

  ret = (NetIOCounterInfo *)calloc(1, sizeof(NetIOCounterInfo));
  counters = (NetIOCounters *)calloc(15, sizeof(NetIOCounters));
  line = (char *)calloc(200, sizeof(char));
  check_mem(line);
  check_mem(counters);
  nc = counters;
  check_mem(ret);
  fp = fopen("/proc/net/dev", "r");
  check(fp, "Couldn't open /proc/net/dev");

  while (fgets(line, 150, fp) != NULL) {
    if (i++ < 2)
      continue;

    ninterfaces++;

    tmp = strtok(line, " \n:"); /* Name */
    nc->name = strdup(tmp);

    tmp = strtok(NULL, " \n"); /* Bytes received 0*/
    nc->bytes_recv = strtoul(tmp, NULL, 10);

    tmp = strtok(NULL, " \n"); /* Packets received 1 */
    nc->packets_recv = strtoul(tmp, NULL, 10);

    tmp = strtok(NULL, " \n"); /* Errors in 2 */
    nc->errin = strtoul(tmp, NULL, 10);

    tmp = strtok(NULL, " \n"); /* Drops in 3 */
    nc->dropin = strtoul(tmp, NULL, 10);

    for (i = 0; i < 4; i++)
      tmp = strtok(NULL, " \n"); /* Skip  4, 5, 6 and 7*/

    tmp = strtok(NULL, " \n"); /* Bytes sent 8*/
    nc->bytes_sent = strtoul(tmp, NULL, 10);

    tmp = strtok(NULL, " \n"); /* Packets sent 9*/
    nc->packets_sent = strtoul(tmp, NULL, 10);

    tmp = strtok(NULL, " \n"); /* Errors out 9*/
    nc->errout = strtoul(tmp, NULL, 10);

    tmp = strtok(NULL, " \n"); /* Drops out 10*/
    nc->dropout = strtoul(tmp, NULL, 10);

    nc++;
  }
  fclose(fp);
  free(line);
  ret->iocounters = counters;
  ret->nitems = ninterfaces;
  return ret;

error:
  /* TODO: ret not freed here */
  if (fp)
    fclose(fp);
  free(line);
  free(counters);
  free(ret);
  return NULL;
}

void free_net_iocounter_info(NetIOCounterInfo *d) {
  for (uint32_t i = 0; i < d->nitems; i++) {
    free(d->iocounters[i].name);
  }
  free(d->iocounters);
  free(d);
}

UsersInfo *get_users() {
  uint32_t nusers = 100;
  UsersInfo *ret = (UsersInfo *)calloc(1, sizeof(UsersInfo));
  Users *users = (Users *)calloc(nusers, sizeof(Users));
  Users *u = users;
  struct utmp *ut;
  check_mem(ret);
  check_mem(users);

  ret->nitems = 0;
  ret->users = users;

  while (NULL != (ut = getutent())) {
    if (ut->ut_type != USER_PROCESS)
      continue;
    u->username = strdup(ut->ut_user);
    check_mem(u->username);

    u->tty = strdup(ut->ut_line);
    check_mem(u->tty);

    u->hostname = strdup(ut->ut_host);
    check_mem(u->hostname);

    u->tstamp = ut->ut_tv.tv_sec;

    ret->nitems++;
    u++;

    if (ret->nitems ==
        nusers) { /* More users than we've allocated space for. */
      nusers *= 2;
      users = (Users *)realloc(users, sizeof(Users) * nusers);
      check_mem(users);
      ret->users = users;
      u = ret->users + ret->nitems; /* Move the cursor to the correct
                                       value in case the realloc moved
                                       the memory */
    }
  }
  endutent();
  return ret;

error:
  free_users_info(ret);
  return NULL;
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

uint32_t get_boot_time() {
  char *tmp = NULL;
  unsigned long ret = -1;
  FILE *fp = fopen("/proc/stat", "r");
  char *line = NULL;
  check(fp, "Couldn't open /proc/stat");
  line = (char *)calloc(200, sizeof(char));
  check_mem(line);

  while (fgets(line, 150, fp)) {
    if (strncmp(line, "btime", 5) == 0) {
      strtok(line, " ");
      tmp = strtok(NULL, "\n");
      ret = strtoul(tmp, NULL, 10);
      break;
    }
  }
  if ((errno == ERANGE && ret == ULONG_MAX) || (errno != 0 && ret == 0)) {
    check(false, "Couldn't find 'btime' line in /proc/stat");
  }

  fclose(fp);
  free(line);

  return ret;

error:
  if (fp)
    fclose(fp);
  free(line);
  free(tmp);
  return -1;
}

bool virtual_memory(VmemInfo *ret) {
  struct sysinfo info;
  FILE *fp = NULL;
  unsigned long long totalram, freeram, bufferram;
  unsigned long long cached = ULLONG_MAX, active = ULLONG_MAX,
                     inactive = ULLONG_MAX;
  char *line = (char *)calloc(50, sizeof(char));
  check_mem(line);
  check(sysinfo(&info) == 0, "sysinfo failed");
  totalram = info.totalram * info.mem_unit;
  freeram = info.freeram * info.mem_unit;
  bufferram = info.bufferram * info.mem_unit;

  fp = fopen("/proc/meminfo", "r");
  check(fp, "Couldn't open /proc/meminfo");
  while (fgets(line, 40, fp) != NULL) {
    if (strncmp(line, "Cached:", 7) == 0) {
      strtok(line, ":"); /* Drop "Cached:" */
      cached = strtoull(strtok(NULL, " "), NULL, 10) * 1024;
    } else if (strncmp(line, "Inactive:", 9) == 0) {
      strtok(line, ":"); /* Drop "Inactive:" */
      inactive = strtoull(strtok(NULL, " "), NULL, 10) * 1024;
    } else if (strncmp(line, "Active:", 7) == 0) {
      strtok(line, ":"); /* Drop "Active:" */
      active = strtoull(strtok(NULL, " "), NULL, 10) * 1024;
    }
  }
  if (cached == ULLONG_MAX || active == ULLONG_MAX || inactive == ULLONG_MAX) {
    log_warn("Couldn't determine 'cached', 'active' and 'inactive' memory "
             "stats. Setting them to 0");
    cached = active = inactive = 0;
  }
  fclose(fp);
  free(line);

  ret->total = totalram;
  ret->available = freeram + bufferram + cached;
  ret->percent = percentage((totalram - ret->available), totalram);
  ret->used = totalram - freeram;
  ret->free = freeram;
  ret->active = active;
  ret->inactive = inactive;
  ret->buffers = bufferram;
  ret->cached = cached;

  return true;
error:
  if (fp)
    fclose(fp);
  free(line);
  return false;
}

bool swap_memory(SwapMemInfo *ret) {
  struct sysinfo info;
  FILE *fp = NULL;
  char *line = NULL;

  unsigned long totalswap, freeswap, usedswap;
  unsigned long sin = ULONG_MAX, sout = ULONG_MAX;
  check(sysinfo(&info) == 0, "sysinfo failed");

  totalswap = info.totalswap;
  freeswap = info.freeswap;
  usedswap = totalswap - freeswap;

  fp = fopen("/proc/vmstat", "r");
  check(fp, "Couldn't open /proc/vmstat");

  line = (char *)calloc(50, sizeof(char));
  check_mem(line);

  while (fgets(line, 40, fp) != NULL) {
    if (strncmp(line, "pswpin", 6) == 0) {
      sin = strtoul(line + 7, NULL, 10) * 4 * 1024;
    }
    if (strncmp(line, "pswpout", 7) == 0) {
      sout = strtoul(line + 8, NULL, 10) * 4 * 1024;
    }
  }
  if (sin == ULONG_MAX || sout == ULONG_MAX) {
    log_warn(
        "Couldn't determine 'sin' and 'sout' swap stats. Setting them to 0");
    sout = sin = 0;
  }
  fclose(fp);
  free(line);

  ret->total = totalswap;
  ret->used = usedswap;
  ret->free = freeswap;
  ret->percent = percentage(usedswap, totalswap);
  ret->sin = sin;
  ret->sout = sout;

  return true;
error:
  if (fp)
    fclose(fp);
  free(line);
  return false;
}

CpuTimes *cpu_times(bool percpu) {
  FILE *fp = NULL;
  char *line = NULL;
  CpuTimes *ret = NULL;
  int i = 0;
  fp = fopen("/proc/stat", "r");
  check(fp, "Couldn't open /proc/stat");
  line = (char *)calloc(150, sizeof(char));
  check_mem(line);

  if (!percpu) {
    /* The cumulative time is the first line */
    check(fgets(line, 140, fp), "Couldn't read from /proc/stat");
    ret = (CpuTimes *)calloc(1, sizeof(CpuTimes));
    check(parse_cpu_times(line, ret) == 0,
          "Error while parsing /proc/stat line for cpu times");
    fclose(fp);
    free(line);
    return ret;
  } else {
    check(fgets(line, 140, fp),
          "Couldn't read from /proc/stat"); /* Drop the first line */
    ret = (CpuTimes *)calloc(20, sizeof(CpuTimes));
    check_mem(ret);

    while (1) { /* Dropped the first line, read the rest */
      check(fgets(line, 140, fp), "Couldn't read from /proc/stat");
      if (strncmp(line, "cpu", 3) != 0) {
        break;
      }
      check(parse_cpu_times(line, ret + i) == 0,
            "Error while parsing /proc/stat line for cpu times");
      i++;
      /* TODO: Reallocate if we have more than 20 CPUs */
    }
  }
  fclose(fp);
  free(line);
  return ret;
error:
  if (fp)
    fclose(fp);
  free(line);
  free(ret);
  return NULL;
}

CpuTimes *cpu_times_percent(bool percpu, CpuTimes *prev_times) {
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
  free(current);
  return NULL;
}

double *cpu_util_percent(bool percpu, CpuTimes *prev_times) {
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
  free(current);
  return NULL;
}

uint32_t cpu_count(bool logical) {
  long ret = -1;
  if (logical) {
    return logical_cpu_count();
  } else {
    return physical_cpu_count();
  }
  return ret;
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
  unsigned int *uids = NULL;
  unsigned int *gids = NULL;
  retval->pid = pid;
  retval->ppid = get_ppid(pid);
  retval->name = get_procname(pid);
  retval->exe = get_exe(pid);
  retval->cmdline = get_cmdline(pid);
  retval->create_time = get_create_time(pid);
  uids = get_ids(pid, "Uid:");
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

  gids = get_ids(pid, "Gid:");
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

void free_process(Process *p) {
  free(p->name);
  free(p->exe);
  free(p->cmdline);
  free(p->username);
  free(p->terminal);
  free(p);
}

/*
  The following function is an ugly workaround to ensure that coverage
  data can be manually flushed to disk during py.test invocations. If
  this is not done, we cannot measure the coverage information. More details
  in http://... link_to_bug...

 */
void gcov_flush(void) { __gcov_flush(); }
