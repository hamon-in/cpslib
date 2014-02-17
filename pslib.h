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

typedef struct {
  unsigned long total;
  unsigned long available;
  float percent;
  unsigned long used;
  unsigned long free;
  unsigned long active;
  unsigned long inactive;
  unsigned long buffers;
  unsigned long cached;
} VmemInfo;

typedef struct {
  unsigned int pid;
  unsigned int ppid;
  char *name;
  char *exe;
  char *cmdline;
  unsigned long create_time;
  unsigned int uid;
  unsigned int euid;
  unsigned int suid;
  unsigned int gid;
  unsigned int egid;
  unsigned int sgid;
  char *username;
  char *terminal;
} Process;

int disk_usage(char [], DiskUsage *);

DiskPartitionInfo *disk_partitions();
void free_disk_partition_info(DiskPartitionInfo *);

DiskIOCounterInfo *disk_io_counters();
void free_disk_iocounter_info(DiskIOCounterInfo *);

NetIOCounterInfo *net_io_counters();
void free_net_iocounter_info(NetIOCounterInfo *);

UsersInfo *get_users();
void free_users_info(UsersInfo *);

unsigned long int get_boot_time();

int virtual_memory(VmemInfo *);

int cpu_count(int);
int logical_cpu_count();
int physical_cpu_count();

Process *get_process(unsigned int);
void free_process(Process *);

#endif

