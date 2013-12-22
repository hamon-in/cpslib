#include <stdlib.h>
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

