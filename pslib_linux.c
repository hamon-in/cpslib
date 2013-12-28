#include <search.h>
#include <ctype.h>
#include <mntent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/statvfs.h>

#include "pslib_linux.h"
#include "common.h"

int
disk_usage(char path[], DiskUsage *ret) 
{
  struct statvfs s;
  statvfs(path, &s); /* TBD: Handle failure conditions properly */

  ret->free = s.f_bavail * s.f_frsize;
  ret->total = s.f_blocks * s.f_frsize;
  ret->used = (s.f_blocks - s.f_bfree ) * s.f_frsize;
  ret->percent = percentage(ret->used, ret->total);

  return 0;
}

DiskPartitionInfo *
disk_partitions()
{
  FILE *file = NULL;
  struct mntent *entry;
  int nparts = 5, c = 0;
  DiskPartition *partitions = (DiskPartition *)calloc(nparts, sizeof(DiskPartition) * nparts);
  DiskPartitionInfo *ret = (DiskPartitionInfo *)calloc(1, sizeof(DiskPartitionInfo));
  file = setmntent(MOUNTED, "r"); /* TBD: Handle null file */
  
  while ((entry = getmntent(file))) { /* TBD: Failure conditions here */
    partitions[c].device  = strdup(entry->mnt_fsname);
    partitions[c].mountpoint = strdup(entry->mnt_dir);
    partitions[c].fstype = strdup(entry->mnt_type);
    partitions[c].opts = strdup(entry->mnt_opts);
    c++;
    if (c == nparts) {
      nparts *= 2;
      partitions = realloc(partitions, sizeof(DiskPartition) * nparts); /* Handle allocation failures here */
    }
  }

  ret->nitems = c;
  ret->partitions = partitions;
  
  endmntent(file);
  return ret;
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
  char *tmp;
  int i = 0, nparts = 0;
  size_t nmemb;
  char *line = (char *)calloc(150, sizeof(char) * 150);
  char **partitions = (char **)calloc(30, sizeof(char *) * 30);
  DiskIOCounters *counters = NULL;
  DiskIOCounters *ci = NULL;
  DiskIOCounterInfo *ret = (DiskIOCounterInfo *)calloc(1, sizeof(DiskIOCounterInfo));
  while (fgets(line, 40, fp) != NULL) {
    if (i++ < 2) {continue;}   
    tmp = strtok(line, " \n"); /* major number */
    tmp = strtok(NULL, " \n"); /* minor number */
    tmp = strtok(NULL, " \n"); /* Number of blocks */
    tmp = strtok(NULL, " \n"); /* Name */
    if (isdigit(tmp[strlen(tmp)-1])) { /* Only partitions (i.e. skip things like 'sda')*/
      /* TBD: Skipping handling http://code.google.com/p/psutil/issues/detail?id=338 for now */
      partitions[nparts] = strndup(tmp, 10); 
      nparts++; /* TBD: We can't handle more than 30 partitions now */
    }
  }
  fclose(fp);
  
  nmemb = nparts;
  counters = (DiskIOCounters *)calloc(nparts, sizeof(DiskIOCounters) * nparts);
  ci = counters;
  fp = fopen("/proc/diskstats", "r");
  
  while (fgets(line, 120, fp) != NULL) {
    tmp = strtok(line, " \n"); /* major number (skip) */
    tmp = strtok(NULL, " \n"); /* minor number (skip) */
    tmp = strtok(NULL, " \n"); /* name */
    if (lfind(&tmp, partitions, &nmemb, sizeof(char *), str_comp)) {
      ci->name = strdup(tmp);

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

