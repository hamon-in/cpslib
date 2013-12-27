#include <stdio.h>
#include "pslib_linux.h"

int
main()
{
  int i;
  DiskUsage du;
  DiskPartitionInfo *dp;
  printf(" Disk usage \n");
  disk_usage("/", &du);
  printf("total: %ld\nused: %ld\nfree: %ld\npercent: %f\n", du.total, du.used, du.free, du.percent);

  printf(" Disk partitions \n");
  dp = disk_partitions();
  printf("Partitions : %d\n", dp->nitems);
  for(i = 0; i < dp->nitems; i++)
    printf("%s %s %s %s\n", 
           dp->partitions[i].device,
           dp->partitions[i].mountpoint,
           dp->partitions[i].fstype,
           dp->partitions[i].opts);
    
  free_disk_partition_info(dp);
  return 0;
}


