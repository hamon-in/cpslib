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
""")

if __name__ == '__main__':
    ffi.compile()
