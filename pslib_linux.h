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
} DiskPartition; /* TBD: Pluralise */

typedef struct {
  int nitems;
  DiskPartition *partitions;
} DiskPartitionInfo;

typedef struct {
  char *name;
  unsigned long readbytes;
  unsigned long writebytes;
  unsigned long reads;
  unsigned long writes;
  unsigned long readtime;
  unsigned long writetime;
} DiskIOCounters;

typedef struct {
  int nitems;
  DiskIOCounters *iocounters;
} DiskIOCounterInfo;

typedef struct {
  char * name;
  unsigned long bytes_sent;
  unsigned long bytes_recv;
  unsigned long packets_sent;
  unsigned long packets_recv;
  unsigned long errin;
  unsigned long errout;
  unsigned long dropin;
  unsigned long dropout;
} NetIOCounters;

typedef struct {
  int nitems;
  NetIOCounters *iocounters;
} NetIOCounterInfo;

typedef struct {
  char *username;
  char *tty;
  char *hostname;
  float tstamp;
} Users;

typedef struct {
  int nitems;
  Users *users;
} UsersInfo;


int disk_usage(char [], DiskUsage *);

DiskPartitionInfo *disk_partitions();
void free_disk_partition_info(DiskPartitionInfo *);

DiskIOCounterInfo *disk_io_counters();
void free_disk_iocounter_info(DiskIOCounterInfo *);

NetIOCounterInfo *net_io_counters();
void free_net_iocounter_info(NetIOCounterInfo *);

UsersInfo *get_users();
void free_users_info(UsersInfo *);

#endif

