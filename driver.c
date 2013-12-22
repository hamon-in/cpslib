#include <stdio.h>
#include "pslib_linux.h"

int
main()
{
  DiskUsage du;
  disk_usage("/", &du);
  printf("total: %ld\nused: %ld\nfree: %ld\npercent: %f\n", du.total, du.used, du.free, du.percent);
  return 0;
}



