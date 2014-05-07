#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include "pslib.h"

void
test_diskusage()
{
  DiskUsage du;
  printf(" Disk usage \n");
  disk_usage("/", &du);
  printf("total: %ld\nused: %ld\nfree: %ld\npercent: %f\n", du.total, du.used, du.free, du.percent);
  printf("\n");
}

void
test_diskpartitioninfo()
{
  int i;
  DiskPartitionInfo *phys_dp, *all_dp;
  printf(" Physical Disk partitions \n");
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

  printf(" All Disk partitions \n");
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

  printf(" Disk IO Counters \n");
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
  printf(" Logged in users \n");
  r = get_users();
  if (! r) {
    printf("Failed \n");
    return;
  }
  printf("Total: %d\n", r->nitems);
  for (i = 0; i < r->nitems; i++) {
    printf("%s  %s   %s   %f\n",
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
  if (t == -1) {
    printf("Aborting\n");
    return;
  }
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
  printf(" Virtual memory\n");
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
  printf(" Swap Memory\n");
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
  CpuTimesInfo *info;
  CpuTimes *r;
  info = cpu_times(0);
  if(! info) {
    printf("Aborting\n");
    return;
  }
  r = *(info->cputimes);
  printf(" CPU times\n");
  printf("User: %.3lf\n", r->user);
  printf("System: %.3lf\n", r->system);
  printf("Idle: %.3lf\n", r->idle);
  printf("Nice: %.3lf\n", r->nice);
  printf("IOWait: %.3lf\n", r->iowait);
  printf("IRQ: %.3lf\n", r->irq);
  printf("SoftIRQ: %.3lf\n", r->softirq);
  printf("Steal: %.3lf\n", r->steal);
  printf("Guest: %.3lf\n", r->guest);
  printf("Guest nice: %.3lf\n", r->guest_nice);
  printf("\n");
  free_cputimes_info(info);
}

void
test_cpu_times_percpu()
{
  CpuTimesInfo *info;
  CpuTimes *r;
  int i;
  info = cpu_times(1);
  if(! info) {
    printf("Aborting\n");
    return;
  }
  printf(" Individual CPU times\n");
  for (i=0; i<info->nitems; i++) {
    r = *(info->cputimes + i);
    printf("CPU %d :: ", i+1);
    printf(" Usr: %.3lf;", r->user);
    printf(" Sys: %.3lf;", r->system);
    printf(" Idle: %.3lf;", r->idle);
    printf(" Nice: %.3lf;", r->nice);
    printf(" IOWait: %.3lf;", r->iowait);
    printf(" IRQ: %.3lf;", r->irq);
    printf(" SoftIRQ: %.3lf;", r->softirq);
    printf(" Steal: %.3lf;", r->steal);
    printf(" Guest: %.3lf;", r->guest);
    printf(" Guest nice: %.3lf\n", r->guest_nice);
  }
  printf("\n");
  free_cputimes_info(info);
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
  printf("Logical : %d\nPhysical : %d\n", logical ,physical);
  printf("\n");
}

void test_process()
{
  pid_t pid = getpid();
  Process *process = get_process(pid);
  printf(" Process information \n");
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
  test_cpu_count();
  test_process();
  return 0;
}
