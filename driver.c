#include <stdio.h>
#include <stdlib.h>
#include "pslib.h"

void
test_boottime()
{
  unsigned long t = get_boot_time();
  printf(" Boot time \n");
  printf("%ld\n\n", t);
}

void
test_cpu_count()
{
  int logical;
  int physical;
  logical = cpu_count(1);
  physical = cpu_count(0);
  printf(" CPU count \n");
  if (logical == -1 || physical == -1) {
    printf("Aborting\n");
    return;
  }
  printf("Logical : %d\nPhysical : %d\n", logical, physical);
  printf("\n");
}

void
test_cpu_times()
{
  CpuTimes *r;
  r = cpu_times(0);
  if(!r) {
    printf("Aborting\n");
    return;
  }
  printf(" CPU times\n");
  printf("User: %.3lf;", r->user);
  printf(" Nice: %.3lf;", r->nice);
  printf(" System: %.3lf;", r->system);
  printf(" Idle: %.3lf;", r->idle);
  printf("\n\n");

  free(r);
}

void
test_cpu_times_percpu()
{
  CpuTimes *r, *c;
  int i, ncpus = cpu_count(1);
  c = r = cpu_times(1);
  if(!r) {
    printf("Aborting\n");
    return;
  }
  printf(" Individual CPU times\n");
  for (i=0; i<ncpus; i++) {
    printf("CPU %d :: ", i+1);
    printf(" Usr: %.3lf;", c->user);
    printf(" Nice: %.3lf;", c->nice);
    printf(" Sys: %.3lf;", c->system);
    printf(" Idle: %.3lf;", c->idle);
    printf("\n");
    c++;
  }
  printf("\n");
  free(r);
}

void
test_getusers()
{
  UsersInfo *r;
  int i;
  printf(" Logged in users \n");
  r = get_users();
  if (! r) {
    printf("Failed \n");
    return;
  }
  printf("Total: %d\n", r->nitems);
  printf("Name\tTerminal Host\tStarted\n");
  for (i = 0; i < r->nitems; i++) {
    printf("%s\t%s\t %s\t%.1f\n",
           r->users[i].username,
           r->users[i].tty,
           r->users[i].hostname,
           r->users[i].tstamp);
  }
  free_users_info(r);
  printf("\n");
}


int
main()
{
//  test_diskusage();
//  test_diskpartitioninfo();
//  test_diskiocounters();
//  test_netiocounters();
  test_getusers();
  test_boottime();
//  test_virtualmeminfo();
//  test_swap();
//
  test_cpu_times();
  test_cpu_times_percpu();
//
//  test_cpu_util_percent();
//  test_cpu_util_percent_percpu();
//
//  test_cpu_times_percent();
//  test_cpu_times_percent_percpu();
//
 test_cpu_count();
//  test_process();
  return 0;
}
