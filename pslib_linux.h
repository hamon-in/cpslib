#ifndef __pslib_linux_h
#define __pslib_linux_h

typedef struct {
  unsigned long total;
  unsigned long used;
  unsigned long free;
  float percent;
} DiskUsage;


int disk_usage(char [], DiskUsage *);
#endif
