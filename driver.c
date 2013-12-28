#include <stdio.h>
#include "pslib_linux.h"

void
test_diskusage()
{
  DiskUsage du;
  printf(" Disk usage \n");
  disk_usage("/", &du);
  printf("total: %ld\nused: %ld\nfree: %ld\npercent: %f\n", du.total, du.used, du.free, du.percent);
  printf("\n");
}

void
test_diskpartitioninfo()
{
  int i;
  DiskPartitionInfo *dp;
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
  printf("\n");
}

void
test_diskiocounters()
{
  DiskIOCounterInfo *d;
  DiskIOCounters *dp;
  d = disk_io_counters();
  printf(" Disk IO Counters \n");
  dp = d->iocounters;
  int i;
  for (i = 0 ; i < d->nitems; i ++) {
    printf("%s: rbytes=%ld,wbytes=%ld,reads=%ld,writes=%ld,rtime=%ld,wtime=%ld\n",
           dp->name,
           dp->readbytes,
           dp->writebytes,
           dp->reads,
           dp->writes,
           dp->readtime,
           dp->writetime);
    dp++;
  }
  free_disk_iocounter_info(d);
  printf("\n");
}

int
main()
{
  test_diskusage();
  test_diskpartitioninfo();
  test_diskiocounters();
  return 0;
}


