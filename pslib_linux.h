#ifndef __pslib_linux_h
#define __pslib_linux_h

typedef struct {
  unsigned long total;
  unsigned long used;
  unsigned long free;
  float percent;
} DiskUsage;

typedef struct {
  char *device;
  char *mountpoint;
  char *fstype;
  char *opts;
} DiskPartition;

typedef struct {
  int nitems;
  DiskPartition *partitions;
} DiskPartitionInfo;

int disk_usage(char [], DiskUsage *);
int disk_partitions(DiskPartitionInfo *);
void free_disk_partition_info(DiskPartitionInfo *);
#endif

