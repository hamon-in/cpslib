#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include "pslib.h"

void
test_diskusage()
{
  DiskUsage du;
  printf(" -- disk_usage \n");
  disk_usage("/", &du);
  printf("total: %lud\nused: %lud\nfree: %lud\npercent: %f\n", du.total, du.used, du.free, du.percent);
  printf("\n");
}

void
test_diskpartitioninfo()
{
  int i;
  DiskPartitionInfo *phys_dp, *all_dp;
  printf(" -- disk_partitions \n");
  phys_dp = disk_partitions(1);
  if (!phys_dp) {
    printf("Aborting\n");
    return;
  }
  printf("Partitions : %d\n", phys_dp->nitems);
  for(i = 0; i < phys_dp->nitems; i++)
    printf("%s %s %s %s\n",
           phys_dp->partitions[i].device,
           phys_dp->partitions[i].mountpoint,
           phys_dp->partitions[i].fstype,
           phys_dp->partitions[i].opts);

  free_disk_partition_info(phys_dp);

  printf("\n");

  printf(" -- disk_partitions \n");
  all_dp = disk_partitions(0);
  if (!all_dp) {
    printf("Aborting\n");
    return;
  }
  printf("Partitions : %d\n", all_dp->nitems);
  for(i = 0; i < all_dp->nitems; i++)
    printf("%s %s %s %s\n",
           all_dp->partitions[i].device,
           all_dp->partitions[i].mountpoint,
           all_dp->partitions[i].fstype,
           all_dp->partitions[i].opts);

  free_disk_partition_info(all_dp);
  printf("\n");
}

void
test_diskiocounters()
{
  DiskIOCounterInfo *d;
  DiskIOCounters *dp;
  d = disk_io_counters();
  if (! d) {
    printf("Aborting");
    return;
  }

  printf(" -- disk_io_counters \n");
  dp = d->iocounters;
  int i;
  for (i = 0 ; i < d->nitems; i ++) {
    printf("%s: rbytes=%ld,wbytes=%ld,reads=%ld,writes=%ld,rtime=%ld,wtime=%ld\n",
           dp->name,
           dp->readbytes,
           dp->writebytes,
           dp->reads,
           dp->writes,
           dp->readtime,
           dp->writetime);
    dp++;
  }
  free_disk_iocounter_info(d);
  printf("\n");
}

void
test_netiocounters()
{
  NetIOCounterInfo *n;
  NetIOCounters *dp;
  int i;
  n = net_io_counters();
  dp = n->iocounters;
  printf("Interfaces: %d\n", n->nitems);
  for (i=0; i < n->nitems; i++) {
    printf("%s: bytes_sent=%ld bytes_rec=%ld packets_sen=%ld packets_rec=%ld erri=%ld errou=%ld dropi=%ld dropou=%ld \n",
           dp->name,
           dp->bytes_sent,
           dp->bytes_recv,
           dp->packets_sent,
           dp->packets_recv,
           dp->errin,
           dp->errout,
           dp->dropin,
           dp->dropout);
    dp++;
  }
  free_net_iocounter_info(n);
  printf("\n");
}

void
test_getusers()
{
  UsersInfo *r;
  int i;
  printf(" -- users\n");
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

void
test_boottime()
{
  unsigned long t = get_boot_time();
  printf(" Boot time \n");
  printf("%ld\n\n", t);
}

void
test_virtualmeminfo()
{
  VmemInfo r;
  int t = virtual_memory(&r);
  if (t == -1) {
    printf("Aborting\n");
    return;
  }
  printf(" -- virtual_memory\n");
  printf("Total: %lu\n", r.total);
  printf("Available: %lu\n", r.available);
  printf("Percent: %f\n", r.percent);
  printf("Used: %lu\n", r.used);
  printf("Free: %lu\n", r.free);
  printf("Active: %lu\n", r.active);
  printf("Inactive: %lu\n", r.inactive);
  printf("Buffers: %lu\n", r.buffers);
  printf("Cached: %lu\n", r.cached);
  printf("\n");
}

void
test_swap()
{
  SwapMemInfo r;
  int t = swap_memory(&r);
  if (t == -1) {
    printf("Aborting\n");
    return;
  }
  printf(" -- swap_memory\n");
  printf("Total: %lu\n", r.total);
  printf("Used: %lu\n", r.used);
  printf("Free: %lu\n", r.free);
  printf("Percent: %f\n", r.percent);
  printf("Sin: %lu\n", r.sin);
  printf("Sout: %lu\n", r.sout);
  printf("\n");
}

void
test_cpu_times()
{
  CpuTimes *r;
  r = cpu_times(0);
  if(! r) {
    printf("Aborting\n");
    return;
  }
  printf(" -- cpu_times\n");
  printf("User: %.3lf;", r->user);
  printf(" System: %.3lf;", r->system);
  printf(" Idle: %.3lf;", r->idle);
  printf(" Nice: %.3lf;", r->nice);
  printf(" IOWait: %.3lf;", r->iowait);
  printf(" IRQ: %.3lf;", r->irq);
  printf(" SoftIRQ: %.3lf;", r->softirq);
  printf(" Steal: %.3lf;", r->steal);
  printf(" Guest: %.3lf;", r->guest);
  printf(" Guest nice: %.3lf\n", r->guest_nice);
  printf("\n");

  free(r);
}

void
test_cpu_times_percpu()
{
  CpuTimes *r, *c;
  int i, ncpus = cpu_count(1);
  c = r = cpu_times(1);
  if(! r) {
    printf("Aborting\n");
    return;
  }
  printf(" -- cpu_times\n");
  for (i=0; i<ncpus; i++) {
    printf("CPU %d :: ", i+1);
    printf(" Usr: %.3lf;", c->user);
    printf(" Sys: %.3lf;", c->system);
    printf(" Idle: %.3lf;", c->idle);
    printf(" Nice: %.3lf;", c->nice);
    printf(" IOWait: %.3lf;", c->iowait);
    printf(" IRQ: %.3lf;", c->irq);
    printf(" SoftIRQ: %.3lf;", c->softirq);
    printf(" Steal: %.3lf;", c->steal);
    printf(" Guest: %.3lf;", c->guest);
    printf(" Guest nice: %.3lf\n", c->guest_nice);
    c++;
  }
  printf("\n");
  free(r);
}

  

void
test_cpu_times_percent()
{
  CpuTimes *info;
  CpuTimes *ret;
  info = cpu_times(0);
  if(! info) {
    printf("Aborting\n");
    return;
  }
  usleep(100000);
  ret = cpu_times_percent(0, info); /* Actual percentages since last call */
  if (! ret) {
    printf("Error while computing utilisation percentage\n");
    return;
  }
  printf(" -- cpu_times_percent\n");
  printf("CPU times as percentage of total (0.1 second sample)\n");
  printf("Usr: %.3lf;", ret->user);
  printf(" Sys: %.3lf;", ret->system);
  printf(" Idle: %.3lf;", ret->idle);
  printf(" Nice: %.3lf;", ret->nice);
  printf(" IOWait: %.3lf;", ret->iowait);
  printf(" IRQ: %.3lf;", ret->irq);
  printf(" SoftIRQ: %.3lf;", ret->softirq);
  printf(" Steal: %.3lf;", ret->steal);
  printf(" Guest: %.3lf;", ret->guest);
  printf(" Guest nice: %.3lf\n", ret->guest_nice);
  printf("\n");
  free(info);
  free(ret);
}

void
test_cpu_times_percent_percpu()
{
  CpuTimes *info, *last, *r;
  int i, ncpus = cpu_count(1);
  last = cpu_times(1);
  if(! last) {
    printf("Aborting\n");
    return;
  }
  usleep(100000);
  r = info = cpu_times_percent(1, last); /* Actual percentages since last call */
  if (! info) {
    printf("Error while computing utilisation percentage\n");
    return;
  }

  printf(" -- cpu_times_percent\n");
  printf("CPU times as percentage of total per CPU (0.1 second sample)\n");
  for (i=0; i<ncpus; i++) {
    printf("CPU %d :: ", i+1);
    printf("Usr: %.3lf;", info->user);
    printf(" Sys: %.3lf;", info->system);
    printf(" Idle: %.3lf;", info->idle);
    printf(" Nice: %.3lf;", info->nice);
    printf(" IOWait: %.3lf;", info->iowait);
    printf(" IRQ: %.3lf;", info->irq);
    printf(" SoftIRQ: %.3lf;", info->softirq);
    printf(" Steal: %.3lf;", info->steal);
    printf(" Guest: %.3lf;", info->guest);
    printf(" Guest nice: %.3lf\n", info->guest_nice);
    info++;
  }

  printf("\n");
  free(r);
  free(last);
}


void 
test_cpu_util_percent()
{
  CpuTimes *info;
  double *utilisation;
  info = cpu_times(0);

  if(! info) {
    printf("Aborting\n");
    return;
  }

  usleep(100000);
  utilisation = cpu_util_percent(0, info);
  printf(" -- cpu_util_percent\n");
  printf("%f\n", *utilisation);
  printf("\n");

  free(utilisation);
  free(info);
}

void 
test_cpu_util_percent_percpu()
{
  CpuTimes *info;
  double *percentages;
  int i, ncpus = cpu_count(1);
  info = cpu_times(1);

  if(! info) {
    printf("Aborting\n");
    return;
  }

  usleep(100000);
  percentages = cpu_util_percent(1, info);
  printf(" -- cpu_util_percent\n");
  for (i=0; i<ncpus; i++) {
    printf("Cpu #%d : %f\n", i, percentages[i]);
  }

  printf("\n");

  free(percentages);
  free(info);
}


void
test_cpu_count()
{
  int logical;
  int physical;
  logical = cpu_count(1);
  physical = cpu_count(0);
  printf(" -- cpu_count \n");
  if (logical == -1 || physical == -1) {
    printf("Aborting\n");
    return;
  }
  printf("Logical : %d\nPhysical : %d\n", logical ,physical);
  printf("\n");
}

void test_process()
{
  pid_t pid = getpid();
  Process *process = get_process(pid);
  printf(" -- process \n");
  printf("pid %d\n",process->pid);
  printf("ppid %d\n",process->ppid);
  printf("name %s\n",process->name);
  printf("exe %s\n",process->exe);
  printf("cmdline %s\n",process->cmdline);
  printf("Real uid %d\n", process->uid);
  printf("Effective uid %d\n", process->euid);
  printf("Saved uid %d\n", process->suid);
  printf("Real gid %d\n", process->gid);
  printf("Effective gid %d\n", process->egid);
  printf("Saved gid %d\n", process->sgid);
  printf("Username %s\n", process->username);
  printf("Terminal %s\n", process->terminal);
  printf("\n");
  free_process(process);
}

int
main()
{
  test_diskusage();
  test_diskpartitioninfo();
  test_diskiocounters();
  test_netiocounters();
  test_getusers();
  test_boottime();
  test_virtualmeminfo();
  test_swap();

  test_cpu_times();
  test_cpu_times_percpu();

  test_cpu_util_percent();
  test_cpu_util_percent_percpu();

  test_cpu_times_percent();
  test_cpu_times_percent_percpu();

  test_cpu_count();
  test_process();
  return 0;
}
