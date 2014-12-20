#include <stdio.h>
#include <sys/types.h>
#include <sys/sysctl.h>

#include "pslib.h"
#include "common.h"

/* TBD : Generic function to get field from a line in a file that starts with something */

/* Internal functions */

/* Public functions */

unsigned long int
get_boot_time()
{
  /* read KERN_BOOTIME */
  int mib[2] = { CTL_KERN, KERN_BOOTTIME };
  struct timeval result;
  size_t result_len = sizeof result;
  time_t boot_time = 0;

  check(sysctl(mib, 2, &result, &result_len, NULL, 0) != -1, "syscall failed");
  boot_time = result.tv_sec;
  return (unsigned long int)boot_time;

error:
  return -1;
}
