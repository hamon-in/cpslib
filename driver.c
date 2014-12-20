#include <stdio.h>
#include "pslib.h"

void
test_boottime()
{
  unsigned long t = get_boot_time();
  printf(" Boot time \n");
  if (t == -1) {
    printf("Aborting\n");
    return;
  }
  printf("%ld\n\n", t);
}

int
main()
{
//  test_diskusage();
//  test_diskpartitioninfo();
//  test_diskiocounters();
//  test_netiocounters();
//  test_getusers();
  test_boottime();
//  test_virtualmeminfo();
//  test_swap();
//
//  test_cpu_times();
//  test_cpu_times_percpu();
//
//  test_cpu_util_percent();
//  test_cpu_util_percent_percpu();
//
//  test_cpu_times_percent();
//  test_cpu_times_percent_percpu();
//
//  test_cpu_count();
//  test_process();
  return 0;
}
