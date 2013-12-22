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

int 
disk_partitions(DiskPartitionInfo *ret)
{
  FILE *file = NULL;
  struct mntent *entry;
  int nparts = 5, c = 0;
  DiskPartition *partitions = (DiskPartition *)malloc(sizeof(DiskPartition) * nparts);

  memset(partitions, 0, sizeof(DiskPartition) * nparts); /* TBD: Handle allocation failure */
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
  return 0;
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
}
