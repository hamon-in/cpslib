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
  DiskPartitionInfo *dp;
  printf(" Disk partitions \n");
  dp = disk_partitions();
  if (!dp) {
    printf("Aborting\n");
    return;
  }
  printf("Partitions : %d\n", dp->nitems);
  for(i = 0; i < dp->nitems; i++)
    printf("%s %s %s %s\n", 
           dp->partitions[i].device,
           dp->partitions[i].mountpoint,
           dp->partitions[i].fstype,
           dp->partitions[i].opts);
    
  free_disk_partition_info(dp);  
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
  SwapMem r;
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
  CpuTimes r;
  int t = cpu_times(&r);
  if(t < 0) {
    printf("Aborting\n");
    return;
  }

  printf(" CPU times\n");
  printf("User: %.3lf\n", r.user);
  printf("System: %.3lf\n", r.system);
  printf("Idle: %.3lf\n", r.idle);
  printf("Nice: %.3lf\n", r.nice);
  printf("IOWait: %.3lf\n", r.iowait);
  printf("IRQ: %.3lf\n", r.irq);
  printf("SoftIRQ: %.3lf\n", r.softirq);
  printf("Steal: %.3lf\n", r.steal);
  printf("Guest: %.3lf\n", r.guest);
  printf("Guest nice: %.3lf\n", r.guest_nice);
  printf("\n");
}

void
test_cpu_times_percent()
{
  CpuTimes r;
  int t = cpu_times_percent(&r);
  if(t < 0) {
    printf("Aborting\n");
    return;
  }

  printf(" CPU times in percent\n");
  printf("User: %.3lf\n", r.user);
  printf("System: %.3lf\n", r.system);
  printf("Idle: %.3lf\n", r.idle);
  printf("Nice: %.3lf\n", r.nice);
  printf("IOWait: %.3lf\n", r.iowait);
  printf("IRQ: %.3lf\n", r.irq);
  printf("SoftIRQ: %.3lf\n", r.softirq);
  printf("Steal: %.3lf\n", r.steal);
  printf("Guest: %.3lf\n", r.guest);
  printf("Guest nice: %.3lf\n", r.guest_nice);
  printf("\n");
}

void
test_cpu_times_percent_per_cpu() {
  CpuTimes *r = NULL; 

  int count = cpu_times_percent_per_cpu(&r);
  if(count < 0) {
    printf("Aborting test cpu_times_percent_per_cpu\n");
    return;
  }

  printf(" CPU time percent per CPU\n");
  int i;
  for(i = 0;i < count; i++) {
    CpuTimes *cur = r+i;
    printf("CPU%d: ", i);
    printf("User: %.2lf, System: %.2lf, Idle: %.3lf, ", cur->user, cur->system, cur->idle);
    printf("Nice: %.2lf, IOWait: %.2lf, IRQ: %.2lf, ", cur->nice, cur->iowait, cur->irq);
    printf("SoftIRQ: %.2lf, Steal: %.2lf, ", cur->softirq, cur->steal);
    printf("Guest: %.2lf, Guest nice: %.2lf)\n", cur->guest, cur->guest_nice);
  }
}

void
test_cpu_percent() {
  double r = cpu_percent();
  printf("\nCPU Percent: %f\n\n", r);
}

void
test_cpu_percent_per_cpu() {
  double *r = NULL;
  int count = cpu_percent_per_cpu(&r);
  if(count < 0) {
    printf("Aborting test cpu_percent_per_cpu\n");
    return;
  }

  printf(" CPU percent per CPU\n");
  int i;
  for(i = 0;i < count; i++) {
    printf("CPU%d: %f\n", i, r[i]);
  }
  printf("\n");
  free(r);
}

void
test_cpu_times_per_cpu() {
  CpuTimes* r = NULL;
  int count = cpu_times_per_cpu(&r);
  if(count < 0) {
    printf("Aborting test cpu_times_per_cpu\n");
    return;
  }

  printf(" CPU times per CPU\n");

  int i;
  for (i = 0;i < count; i++) {
    CpuTimes cur = r[i];
    printf("CPU%d: (", i);
    printf("User: %.2lf, System: %.2lf, Idle: %.3lf, ", cur.user, cur.system, cur.idle);
    printf("Nice: %.2lf, IOWait: %.2lf, IRQ: %.2lf, ", cur.nice, cur.iowait, cur.irq);
    printf("SoftIRQ: %.2lf, Steal: %.2lf, ", cur.softirq, cur.steal);
    printf("Guest: %.2lf, Guest nice: %.2lf)\n", cur.guest, cur.guest_nice);
  }
  printf("\n\n");
  free(r);
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
  test_cpu_times_per_cpu();
  test_cpu_times_percent();
  test_cpu_times_percent_per_cpu();
  test_cpu_percent();
  test_cpu_percent_per_cpu();
  test_cpu_count();
  test_process();
  return 0;
}


