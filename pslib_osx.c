#include <stdio.h>
#include <sys/types.h>
#include <sys/sysctl.h>

#include "pslib.h"
#include "common.h"

/* TBD : Generic function to get field from a line in a file that starts with something */

/* Internal functions */

static int
logical_cpu_count()
{
  return cpu_count(1);
}

static int
physical_cpu_count()
{
  return cpu_count(0);
}


/* Public functions */

unsigned long int
get_boot_time()
{
  /* read KERN_BOOTIME */
  int mib[2] = { CTL_KERN, KERN_BOOTTIME };
  struct timeval result;
  size_t len = sizeof result;
  time_t boot_time = 0;

  check(sysctl(mib, 2, &result, &len, NULL, 0) != -1, "sysctl failed");
  boot_time = result.tv_sec;
  return (unsigned long int)boot_time;

error:
  return -1;
}


int
cpu_count(int logical)
{
  int ncpu;
  size_t len = sizeof(ncpu);

  if (logical) {
    check(sysctlbyname("hw.logicalcpu", &ncpu, &len, NULL, 0) != -1, "sysctl failed");
  } else {
    check(sysctlbyname("hw.physicalcpu", &ncpu, &len, NULL, 0) != -1, "sysctl failed");
  }

  return ncpu;

error:
  return -1;
}
