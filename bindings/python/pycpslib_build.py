# pycpslib.py

import os

from cffi import FFI

ffi = FFI()
project_root = os.path.abspath("../../")

ffi.set_source("pycpslib",
               """#include <stdio.h>
               #include <stdlib.h>
               #include <sys/types.h>
               #include <unistd.h>
               #include "pslib.h"
               """,
               libraries = ["pslib"],
               library_dirs = [project_root],
               include_dirs = [project_root])


ffi.cdef("""
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

DiskPartitionInfo *disk_partitions(int);

typedef struct {
  unsigned long total;
  unsigned long used;
  unsigned long free;
  float percent;
} DiskUsage;

int disk_usage(char [], DiskUsage *);

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

DiskIOCounterInfo *disk_io_counters(void);

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

NetIOCounterInfo *net_io_counters(void);

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

UsersInfo *get_users(void); 

long int get_boot_time(void); 

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

int virtual_memory(VmemInfo *); 

typedef struct {
  unsigned long total;
  unsigned long used;
  unsigned long free;
  float percent;
  unsigned long sin;
  unsigned long sout;
} SwapMemInfo;

int swap_memory(SwapMemInfo *);

typedef struct {
  double user;
  double system;
  double idle;
  double nice;
  double iowait;
  double irq;
  double softirq;
  double steal;
  double guest;
  double guest_nice;
} CpuTimes;

CpuTimes *cpu_times(int);
""")

if __name__ == '__main__':
    ffi.compile()
