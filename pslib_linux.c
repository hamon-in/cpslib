#include <ctype.h>
#include <mntent.h>
#include <search.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/statvfs.h>
#include <utmp.h>

#include "pslib_linux.h"
#include "common.h"

int
disk_usage(char path[], DiskUsage *ret) 
{
  struct statvfs s;
  int r;
  r = statvfs(path, &s); /* TBD: Handle failure conditions properly */
  check(!r, "Error in calling statvfs for %s", path);
  ret->free = s.f_bavail * s.f_frsize;
  ret->total = s.f_blocks * s.f_frsize;
  ret->used = (s.f_blocks - s.f_bfree ) * s.f_frsize;
  ret->percent = percentage(ret->used, ret->total);

  return 0;
 error:
  return -1;
}

DiskPartitionInfo *
disk_partitions()
{
  FILE *file = NULL;
  struct mntent *entry;
  int nparts = 5, c = 0;
  DiskPartition *partitions = (DiskPartition *)calloc(nparts, sizeof(DiskPartition));
  DiskPartitionInfo *ret = (DiskPartitionInfo *)calloc(1, sizeof(DiskPartitionInfo));
  check_mem(partitions);
  check_mem(ret);

  file = setmntent(MOUNTED, "r");
  check(file, "Couldn't open %s", MOUNTED);

  while ((entry = getmntent(file))) { 
    partitions[c].device  = strdup(entry->mnt_fsname); /* TBD: Use a moving pointer  */
    partitions[c].mountpoint = strdup(entry->mnt_dir); /* here rather than this */
    partitions[c].fstype = strdup(entry->mnt_type);    /* indexing */
    partitions[c].opts = strdup(entry->mnt_opts);
    c++;
    if (c == nparts) {
      nparts *= 2;
      partitions = realloc(partitions, sizeof(DiskPartition) * nparts);
      check_mem(partitions);
    }
  }

  ret->nitems = c;
  ret->partitions = partitions;
  
  endmntent(file);
  return ret;
 error:
  /* TBD: ret not freed here It might be a good idea to just delegate
     the freeing to the public freeing function. For this to work,
     nitems should be correct. If we maintain that, we might be able
     to drop the `c`. This holds true for all the Info style
     structures. In short, make this the same as get_users.
  */
  return NULL;
}

void
free_disk_partition_info(DiskPartitionInfo *dp)
{
  int i;
  for(i = 0; i < dp->nitems; i++) {
    free(dp->partitions[i].device);
    free(dp->partitions[i].mountpoint);
    free(dp->partitions[i].fstype);
    free(dp->partitions[i].opts);
  }
  free(dp->partitions);
  free(dp);
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
    }
  }

  for(i = 0; i < nparts; i++)
    free(partitions[i]);
  free(partitions);
  free(line);
  fclose(fp);

  ret->nitems = nparts;
  ret->iocounters = counters;
  return ret;
 error:
  return NULL;
}

void
free_disk_iocounter_info(DiskIOCounterInfo *d)
{
  int i;
  for (i=0; i < d->nitems; i++) {
    free(d->iocounters[i].name);
  }
  free(d->iocounters);
  free(d);
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
