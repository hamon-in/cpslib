#include <ctype.h>
#include <mntent.h>
#include <pwd.h>
#include <search.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/sysinfo.h>
#include <sys/types.h>
#include <unistd.h>
#include <utmp.h>


#include "pslib.h"
#include "common.h"

/* TBD : Generic function to get field from a line in a file that starts with something */

/* Internal functions */

static double 
sum_cpu_time(CpuTimes *t) {
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


static double
calculate_cpu_util_percentage(CpuTimes *t1, CpuTimes *t2)
{
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

static int
logical_cpu_count()
{
  int ret;
  ret = (int)sysconf(_SC_NPROCESSORS_ONLN);
  if (ret == -1) {
    /* TDB: Parse /proc/cpuinfo */
    ;
  }
  return ret;
}

static int
physical_cpu_count()
{
  FILE * fp = NULL;
  size_t s = 100;
  long int ids[s]; /* Assume we don't have more than 100 physical CPUs */
  memset(&ids, -1, sizeof(long int) * 100);
  long int *cid = ids;
  long int id;
  int nprocs = 0;
  fp = fopen("/proc/cpuinfo", "r");
  check(fp, "Couldn't open '/proc/cpuinfo'");
  char *line = (char *)calloc(100, sizeof(char));
  check_mem(line);

  while (fgets(line, 90, fp) != NULL) {
    if (strncasecmp(line, "physical id", 11) == 0){
      strtok(line, ":");
      id = strtol(strtok(NULL, " "), NULL, 10); /* TBD: Assuming that physical id is a number */
      if (! lfind(&id, ids, &s, sizeof(int), int_comp)) { /* TBD: Replace this with lsearch */
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
  if (fp) fclose(fp);
  free(line);
  return -1;
}

char **
get_physical_devices(size_t *ndevices)
{
  FILE *fs = NULL;
  char *line = NULL;
  char **retval = NULL;
  int i;
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
      check(*ndevices < 100, "FIXME: Can't process more than 100 physical devices");
    }
  }
  fclose(fs);
  free(line);
  return retval;
 error:
  if (fs) fclose(fs);
  if (line) free(line);
  if (*ndevices != 0) 
    for (i = 0; i < *ndevices; i++) 
      free(retval[i]);
  return NULL;
}


static unsigned int
get_ppid(unsigned pid)
{
  FILE *fp = NULL;
  unsigned int ppid = -1;
  char *tmp;
  char procfile[50];

  sprintf(procfile,"/proc/%d/status", pid);
  fp = fopen(procfile,"r");
  check(fp, "Couldn't open process status file");
  tmp = grep_awk(fp, "PPid", 1, ":");
  ppid = tmp?strtoul(tmp, NULL, 10):-1;

  check(ppid != -1, "Couldnt' find Ppid in process status file");
  fclose(fp);
  free(tmp);

  return ppid;
 error:
  if (fp) fclose(fp);
  return -1;
}

static char *
get_procname(unsigned pid)
{
  FILE *fp = NULL;
  char *tmp;
  char procfile[50];
  char line[350];

  sprintf(procfile,"/proc/%d/stat", pid);
  fp = fopen(procfile,"r");
  check(fp, "Couldn't open process status file");
  fgets(line, 300, fp);
  fclose(fp);

  tmp = strtok(line, " ");
  tmp = strtok(NULL, " "); /* Name field */
  tmp = squeeze(tmp, "()");

  return strdup(tmp);
 error:
  if (fp) fclose(fp);
  return NULL;
}

static char *
get_exe(unsigned pid)
{
  FILE *fp = NULL;
  char *tmp = NULL;
  char procfile[50];
  ssize_t ret;
  unsigned int bufsize = 1024;
  struct stat buf;

  sprintf(procfile,"/proc/%d/exe", pid);
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
  while(ret == bufsize -1 ) {
    /* Buffer filled. Might be incomplete. Increase size and try again. */
    bufsize *= 2;
    tmp = realloc(tmp, bufsize);
    ret = readlink(procfile, tmp, bufsize - 1);
    check(ret != -1, "Couldn't expand symbolic link");
  }
  tmp[ret] = '\0';
  return tmp;
 error:
  if (fp) fclose(fp);
  if (tmp) free(tmp);
  return NULL;
}

static char *
get_cmdline(unsigned int pid)
{
  FILE *fp = NULL;
  char procfile[50];
  char *contents = NULL;
  size_t size = 0;

  sprintf(procfile,"/proc/%d/cmdline", pid);
  fp = fopen(procfile, "r");
  check(fp, "Couldn't open process cmdline file");
  size = getline(&contents, &size, fp); /*size argument unused since *contents is NULL */
  check(size != -1, "Couldn't read command line from /proc");
  fclose(fp);
  return contents;

 error:
  if (fp) fclose(fp);
  if (contents) free(contents);
  return NULL;
}

static unsigned long
get_create_time(unsigned int pid)
{
  FILE *fp = NULL;
  char procfile[50];
  char *contents = NULL;
  size_t size = 0;

  sprintf(procfile,"/proc/%d/stat", pid);
  fp = fopen(procfile, "r");
  check(fp, "Couldn't open process stat file");
  size = getline(&contents, &size, fp); /* size argument unused */

 error:
  if (fp) fclose(fp);
  if (contents) free(contents);
  return -1;
}

static unsigned int *
get_ids(unsigned int pid, const char *field)
/* field parameter is used to determine which line to parse (Uid or Gid) */
{
  FILE *fp = NULL;
  char *tmp;
  char procfile[50];
  char line[400];
  unsigned int* retval = NULL;

  sprintf(procfile,"/proc/%d/status", pid);
  fp = fopen(procfile,"r");
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
  if (fp) fclose(fp);
  return NULL;
}

static char *
get_username(unsigned int ruid)
{
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

static char *
get_terminal(unsigned int pid)
{
  FILE *fp = NULL;
  char *tmp = NULL;
  char procfile[50];
  ssize_t ret;
  unsigned int bufsize = 1024;

  sprintf(procfile,"/proc/%d/fd/0", pid);
  tmp = (char *)calloc(bufsize, sizeof(char));
  check_mem(tmp);
  ret = readlink(procfile, tmp, bufsize - 1);
  check(ret != -1, "Couldn't expand symbolic link");
  while(ret == bufsize -1 ) {
    /* Buffer filled. Might be incomplete. Increase size and try again. */
    bufsize *= 2;
    tmp = realloc(tmp, bufsize);
    ret = readlink(procfile, tmp, bufsize - 1);
    check(ret != -1, "Couldn't expand symbolic link");
  }
  tmp[ret] = '\0';
  return tmp;
 error:
  if (fp) fclose(fp);
  if (tmp) free(tmp);
  return NULL;
}


int
parse_cpu_times(char* line, CpuTimes *ret) {
  unsigned long values[10] = {};
  double clock_ticks;
  int i;
  char *pos = strtok(line, " ");

  pos = strtok(NULL, " "); // skip cpu
  for(i = 0; i < 10 && pos != NULL; i++) {
    values[i] = strtoul(pos, NULL, 10);
    pos = strtok(NULL, " ");
  }
  check(i == 10, "Couldn't properly parse cpu times")

  clock_ticks = sysconf(_SC_CLK_TCK);
  
  ret->user       = values[0] / clock_ticks;
  ret->nice       = values[1] / clock_ticks;
  ret->system     = values[2] / clock_ticks;
  ret->idle       = values[3] / clock_ticks;
  ret->iowait     = values[4] / clock_ticks;
  ret->irq        = values[5] / clock_ticks;
  ret->softirq    = values[6] / clock_ticks;
  ret->steal      = values[7] / clock_ticks;
  ret->guest      = values[8] / clock_ticks;
  ret->guest_nice = values[9] / clock_ticks;

  return 0;
 error:
  return -1;
}


/* Public functions */
int
disk_usage(char path[], DiskUsage *ret)
{
  struct statvfs s;
  int r;
  r = statvfs(path, &s);
  check(r == 0, "Error in calling statvfs for %s", path);
  ret->free = s.f_bavail * s.f_frsize;
  ret->total = s.f_blocks * s.f_frsize;
  ret->used = (s.f_blocks - s.f_bfree ) * s.f_frsize;
  ret->percent = percentage(ret->used, ret->total);

  return 0;
 error:
  return -1;
}


DiskPartitionInfo *
disk_partitions(int physical)
{
  FILE *file = NULL;
  struct mntent *entry;
  int nparts = 5;
  char **phys_devices = NULL; 
  size_t nphys_devices;
  int i;

  DiskPartition *partitions = (DiskPartition *)calloc(nparts, sizeof(DiskPartition));
  DiskPartitionInfo *ret = (DiskPartitionInfo *)calloc(1, sizeof(DiskPartitionInfo));
  DiskPartition *d = partitions;
  check_mem(partitions);
  check_mem(ret);
  
  ret->nitems = 0;
  ret->partitions = partitions;

  file = setmntent(MOUNTED, "r");
  check(file, "Couldn't open %s", MOUNTED);

  phys_devices = get_physical_devices(&nphys_devices);

  while ((entry = getmntent(file))) {
    if (physical && ! lfind(&entry->mnt_type, phys_devices, &nphys_devices, sizeof(char *), str_comp)) {
      /* Skip this partition since we only need physical partitions */
      continue;
    }
    d->device  = strdup(entry->mnt_fsname);
    d->mountpoint = strdup(entry->mnt_dir);
    d->fstype = strdup(entry->mnt_type);
    d->opts = strdup(entry->mnt_opts);

    ret->nitems ++;
    d++;

    if (ret->nitems == nparts) {
      nparts *= 2;
      partitions = realloc(partitions, sizeof(DiskPartition) * nparts);
      check_mem(partitions);
      ret->partitions = partitions;
      d = ret->partitions + ret->nitems; /* Move the cursor to the correct
                                            value in case the realloc moved
                                            the memory */
    }
  }
  endmntent(file);

  for (i = 0; i < nphys_devices; i++)
    free(phys_devices[i]);
  free(phys_devices);

  return ret;

 error:
  if (file)
    endmntent(file);
  free_disk_partition_info(ret);
  return NULL;
}

void
free_disk_partition_info(DiskPartitionInfo *di)
{
  DiskPartition *d = di->partitions;
  while(di->nitems--) {
    free(d->device);
    free(d->mountpoint);
    free(d->fstype);
    free(d->opts);
    d++;
  }
  free(di->partitions);
  free(di);
}

DiskIOCounterInfo *
disk_io_counters()
{
  const int sector_size = 512;

  FILE *fp = fopen("/proc/partitions", "r");
  check(fp, "Couldn't open /proc/partitions");

  char *tmp;
  int i = 0, nparts = 0;
  size_t nmemb;
  char *line = (char *)calloc(150, sizeof(char));
  char **partitions = (char **)calloc(30, sizeof(char *));
  DiskIOCounters *counters = NULL;
  DiskIOCounters *ci = NULL;
  DiskIOCounterInfo *ret = (DiskIOCounterInfo *)calloc(1, sizeof(DiskIOCounterInfo));
  check_mem(line);
  check_mem(partitions);
  check_mem(ret);
  while (fgets(line, 40, fp) != NULL) {
    if (i++ < 2) {continue;}
    tmp = strtok(line, " \n"); /* major number */
    tmp = strtok(NULL, " \n"); /* minor number */
    tmp = strtok(NULL, " \n"); /* Number of blocks */
    tmp = strtok(NULL, " \n"); /* Name */
    if (isdigit(tmp[strlen(tmp)-1])) { /* Only partitions (i.e. skip things like 'sda')*/
      /* TBD: Skipping handling http://code.google.com/p/psutil/issues/detail?id=338 for now */
      partitions[nparts] = strndup(tmp, 10);
      check_mem(partitions[nparts]);
      nparts++; /* TBD: We can't handle more than 30 partitions now */
    }
  }
  fclose(fp);

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

      ci++; /* TBD: Realloc here if necessary */
      ret->nitems++;
    }
  }

  for(i = 0; i < nparts; i++)
    free(partitions[i]);
  free(partitions);
  free(line);
  fclose(fp);

  return ret;

 error:
  if (fp) fclose(fp);
  if (line) free(line);
  if (partitions) {
    for(i = 0; i < nparts; i++)
      free(partitions[i]);
    free(partitions);
  }
  free_disk_iocounter_info(ret);

  return NULL;
}

void
free_disk_iocounter_info(DiskIOCounterInfo *di)
{
  DiskIOCounters *d = di->iocounters;
  while (di->nitems--) {
    free (d->name);
    d++;
  }
  free(di->iocounters);
  free(di);
}

NetIOCounterInfo *
net_io_counters()
{
  FILE *fp = NULL;
  NetIOCounterInfo *ret = (NetIOCounterInfo *)calloc(1, sizeof(NetIOCounterInfo));
  NetIOCounters *counters = (NetIOCounters *)calloc(15, sizeof(NetIOCounters));
  NetIOCounters *nc = counters;
  int i = 0, ninterfaces = 0;
  char *line = (char *)calloc(200, sizeof(char));
  char *tmp = NULL;
  check_mem(line);
  check_mem(counters);
  check_mem(ret);
  fp = fopen("/proc/net/dev", "r");
  check(fp, "Couldn't open /proc/net/dev");

  while (fgets(line, 150, fp) != NULL) {
    if (i++ < 2) continue;

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
  /* TBD: ret not freed here */
  if (fp) fclose(fp);
  if (line) free(line);
  if (counters) free(counters);
  if (nc) free(nc);
  return NULL;
}

void
free_net_iocounter_info(NetIOCounterInfo *d)
{
  int i;
  for (i=0; i < d->nitems; i++) {
    free(d->iocounters[i].name);
  }
  free(d->iocounters);
  free(d);
}


UsersInfo *
get_users ()
{
  int nusers = 100;
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
    check_mem(u->username);

    u->hostname = strdup(ut->ut_host);
    check_mem(u->hostname);

    u->tstamp = ut->ut_tv.tv_sec;

    ret->nitems++;
    u++;

    if (ret->nitems == nusers) { /* More users than we've allocated space for. */
      nusers *= 2;
      users = realloc(users, sizeof(Users) * nusers);
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

void
free_users_info(UsersInfo * ui)
{
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

unsigned long int
get_boot_time()
{
  FILE *fp = fopen("/proc/stat", "r");
  char *line = (char *)calloc(200, sizeof(char));
  char *tmp = NULL;
  unsigned long ret = -1;
  check(fp, "Couldn't open /proc/stat");
  check_mem(line);

  while (fgets(line, 150, fp)) {
    if (strncmp(line, "btime", 5) == 0) {
      strtok(line, " ");
      tmp = strtok(NULL, "\n");
      ret = strtoul(tmp, NULL, 10);
      break;
    }
  }
  check(ret != -1, "Couldn't find 'btime' line in /proc/stat");
  fclose(fp);
  free(line);

  return ret;

error:
  if (fp) fclose(fp);
  if (line) free(line);
  if (tmp) free(tmp);
  return -1;
}

int
virtual_memory(VmemInfo *ret)
{
  struct sysinfo info;
  FILE *fp = NULL;
  unsigned long long totalram, freeram, bufferram;
  unsigned long long cached = -1, active = -1, inactive = -1;
  char *line = (char *)calloc(50, sizeof(char));
  check_mem(line);
  check(sysinfo(&info) == 0, "sysinfo failed");
  totalram  = info.totalram  * info.mem_unit;
  freeram   = info.freeram  * info.mem_unit;
  bufferram = info.bufferram  * info.mem_unit;

  fp = fopen("/proc/meminfo", "r");
  check(fp, "Couldn't open /proc/meminfo");
  while (fgets(line, 40, fp) != NULL) {
    if (strncmp(line, "Cached:", 7) == 0){
      strtok(line, ":"); /* Drop "Cached:" */
      cached = strtoull(strtok(NULL, " "), NULL, 10);
    }
    if (strncmp(line, "Active:", 7) == 0){
      strtok(line, ":"); /* Drop "Active:" */
      active = strtoull(strtok(NULL, " "), NULL, 10);
    }
    if (strncmp(line, "Inactive:", 7) == 0){
      strtok(line, ":"); /* Drop "Inactive:" */
      inactive = strtoull(strtok(NULL, " "), NULL, 10);
    }
  }
  if (cached == -1 || active == -1 || inactive == -1) {
    log_warn("Couldn't determine 'cached', 'active' and 'inactive' memory stats. Setting them to 0");
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

  return 0;
 error:
  if(fp) fclose(fp);
  if (line) free(line);
  return -1;
}

int 
swap_memory(SwapMemInfo *ret) {
  struct sysinfo info;
  FILE *fp = NULL;
  char *line = NULL;

  unsigned long totalswap, freeswap, usedswap;
  unsigned long sin = -1, sout = -1;
  check(sysinfo(&info) == 0, "sysinfo failed");

  totalswap = info.totalswap;
  freeswap = info.freeswap;
  usedswap = totalswap - freeswap;

  fp = fopen("/proc/vmstat", "r");
  check(fp, "Couldn't open /proc/vmstat");

  line = (char *)calloc(50, sizeof(char));
  check_mem(line);

  while (fgets(line, 40, fp) != NULL) {
    if(strncmp(line, "pswpin", 6) == 0) {
      sin = strtoul(line+7, NULL, 10);
    }
    if (strncmp(line, "pswpout", 7) == 0) {
      sout = strtoul(line+8, NULL, 10);
    }
  }
  if (sin == -1 || sout == -1) {
    log_warn("Couldn't determine 'sin' and 'sout' swap stats. Setting them to 0");
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

  return 0;
error:
  if(fp) fclose(fp);
  if (line) free(line);
  return -1;
}


CpuTimes *
cpu_times(int percpu) {
  FILE *fp = NULL;
  char *line = NULL;
  CpuTimes *ret = NULL;
  int i = 0;
  fp = fopen("/proc/stat", "r");
  check(fp, "Couldn't open /proc/stat");
  line = (char *) calloc(150, sizeof(char));
  check_mem(line);
  
  if (! percpu) {
    /* The cumulative time is the first line */
    fgets(line, 140, fp);
    ret = (CpuTimes *)calloc(1, sizeof(CpuTimes));
    check(parse_cpu_times(line, ret) == 0,
          "Error while parsing /proc/stat line for cpu times");
    fclose(fp);
    free(line);
    return ret;
  } else {
    fgets(line, 140, fp); /* Drop the first line */
    ret = (CpuTimes *)calloc(20, sizeof(CpuTimes));
    check_mem(ret);

    while(1) {
      fgets(line, 140, fp);
      if(strncmp(line, "cpu", 3) != 0)
        break;
      check (parse_cpu_times(line, ret+i) == 0, 
             "Error while parsing /proc/stat line for cpu times");
      i++;
      /* TBD: Reallocate if we have more than 20 CPUs */
    }
  }
  fclose(fp);
  free(line);
  return ret;
error:
  if (fp) fclose(fp);
  if (line) free(line);
  if (ret) free(ret);
  return NULL;
}

double *
cpu_times_percent(int percpu, CpuTimes *prev_times) {
  CpuTimes *current = NULL;
  int i, ncpus = percpu ? cpu_count(1) : 1;
  double *ret;
  check(prev_times, "Need a reference point. prev_times can't be NULL");
  current = cpu_times(percpu);
  check(current, "Couldn't obtain CPU times");
  ret = (double *)calloc(ncpus, sizeof(double));
  check_mem(ret);
  for (i=0; i<ncpus; i++) {
    *(ret+i) = calculate_cpu_util_percentage(prev_times+i, current+i);
  }
  free(current);
  return ret;
 error:
  if (current) free(current);
  return NULL;
}

double * 
cpu_util_percent(int percpu, CpuTimes *prev_times) {
  CpuTimes *current = NULL;
  int i, ncpus = percpu ? cpu_count(1) : 1;
  double *percentage = (double *)calloc(ncpus, sizeof(double));
  check(prev_times, "Need a reference point. prev_times can't be NULL");

  current = cpu_times(percpu);
  check(current, "Couldn't obtain CPU times");

  for (i=0; i<ncpus; i++) {
    percentage[i] = calculate_cpu_util_percentage(prev_times +i, current + i);
  }
  free(current);
  return percentage;
 error:
  if (current) free(current);
  return NULL;
}


int
cpu_count(int logical)
{
  long ret = -1;
  if (logical) {
    return logical_cpu_count();
  } else {
    return physical_cpu_count();
  }
  return ret;
}


Process *
get_process(unsigned pid)
{
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
    retval->username = get_username(retval->uid); /* Uses real uid and not euid */
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
  if (uids) free(uids);
  if (gids) free(gids);
  return retval;
}


void
free_process(Process *p)
{
  free(p->name);
  free(p->exe);
  free(p->cmdline);
  free(p->username);
  free(p->terminal);
  free(p);
}
