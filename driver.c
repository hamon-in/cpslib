#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "pslib.h"

void test_diskusage() {
  DiskUsage du;
  printf(" -- disk_usage \n");
  disk_usage("/", &du);
  printf("/\ntotal: %" PRIu64 "\nused: %" PRIu64 "\nfree: %" PRIu64
         "\npercent: %f\n\n",
         du.total, du.used, du.free, du.percent);
  disk_usage("/etc", &du);
  printf("/etc\ntotal: %" PRIu64 "\nused: %" PRIu64 "\nfree: %" PRIu64
         "\npercent: %f\n\n",
         du.total, du.used, du.free, du.percent);
}

void test_diskpartitioninfo() {
  uint32_t i;
  DiskPartitionInfo *phys_dp, *all_dp;
  printf(" -- disk_partitions (physical) \n");
  phys_dp = disk_partitions(1);
  if (!phys_dp) {
    printf("Aborting\n");
    return;
  }
  printf("Partitions : %d\n", phys_dp->nitems);
  for (i = 0; i < phys_dp->nitems; i++)
    printf("%s %s %s %s\n", phys_dp->partitions[i].device,
           phys_dp->partitions[i].mountpoint, phys_dp->partitions[i].fstype,
           phys_dp->partitions[i].opts);

  free_disk_partition_info(phys_dp);

  printf("\n");

  printf(" -- disk_partitions (all) \n");
  all_dp = disk_partitions(false);
  if (!all_dp) {
    printf("Aborting\n");
    return;
  }
  printf("Partitions : %d\n", all_dp->nitems);
  for (i = 0; i < all_dp->nitems; i++)
    printf("%s %s %s %s\n", all_dp->partitions[i].device,
           all_dp->partitions[i].mountpoint, all_dp->partitions[i].fstype,
           all_dp->partitions[i].opts);

  free_disk_partition_info(all_dp);
  printf("\n");
}

void test_diskiocounters() {
  DiskIOCounterInfo *d;
  DiskIOCounters *dp;
  d = disk_io_counters();
  if (!d) {
    printf("Aborting");
    return;
  }

  printf(" -- disk_io_counters \n");
  dp = d->iocounters;
  uint32_t i;
  for (i = 0; i < d->nitems; i++) {
    printf("%s: \tread_count=%" PRIu64 ", write_count=%" PRIu64 ", \n"
           "\tread_bytes=%" PRIu64 ", write_bytes=%" PRIu64 ", \n"
           "\tread_time=%" PRIu64 ", write_time=%" PRIu64 "\n",
           dp->name, dp->reads, dp->writes, dp->readbytes, dp->writebytes,
           dp->readtime, dp->writetime);
    dp++;
  }
  free_disk_iocounter_info(d);
  printf("\n");
}

void test_netiocounters() {
  NetIOCounterInfo *n;
  NetIOCounters *dp;
  uint32_t i;
  n = net_io_counters();
  dp = n->iocounters;
  printf(" -- net_io_counters (interface count: %d)\n", n->nitems);
  for (i = 0; i < n->nitems; i++) {
    printf("%s: bytes_sent=%" PRIu64 " bytes_rec=%" PRIu64
           " packets_sen=%" PRIu64 " packets_rec=%" PRIu64 " "
           "erri=%" PRIu64 " errou=%" PRIu64 " dropi=%" PRIu64
           " dropou=%" PRIu64 " \n",
           dp->name, dp->bytes_sent, dp->bytes_recv, dp->packets_sent,
           dp->packets_recv, dp->errin, dp->errout, dp->dropin, dp->dropout);
    dp++;
  }
  free_net_iocounter_info(n);
  printf("\n");
}

void test_getusers() {
  UsersInfo *r;
  uint32_t i;
  printf(" -- users \n");
  r = get_users();
  if (!r) {
    printf("Failed \n");
    return;
  }
  printf("Total: %d\n", r->nitems);
  printf("Name\tTerminal Host\tStarted\n");
  for (i = 0; i < r->nitems; i++) {
    printf("%s\t%s\t %s\t%.1f\n", r->users[i].username, r->users[i].tty,
           r->users[i].hostname, r->users[i].tstamp);
  }
  free_users_info(r);
  printf("\n");
}

void test_boottime() {
  uint32_t t = get_boot_time();
  printf(" -- boot_time \n");
  if (t == UINT32_MAX) {
    printf("Aborting\n");
    return;
  }
  printf("%u\n\n", t);
}

void test_virtualmeminfo() {
  VmemInfo r;
  // Empty out to prevent garbage in platform-specific fields
  memset(&r, 0, sizeof(VmemInfo));
  if (!virtual_memory(&r)) {
    printf("Aborting\n");
    return;
  }
  printf(" -- virtual_memory\n");
  printf("Total: %" PRIu64 "\n", r.total);
  printf("Available: %" PRIu64 "\n", r.available);
  printf("Percent: %f\n", r.percent);
  printf("Used: %" PRIu64 "\n", r.used);
  printf("Free: %" PRIu64 "\n", r.free);
  printf("Active: %" PRIu64 "\n", r.active);
  printf("Inactive: %" PRIu64 "\n", r.inactive);
  printf("Buffers: %" PRIu64 "\n", r.buffers);
  printf("Cached: %" PRIu64 "\n", r.cached);
  printf("Wired: %" PRIu64 "\n", r.wired);
  printf("\n");
}

void test_swap() {
  SwapMemInfo r;
  if (!swap_memory(&r)) {
    printf("Aborting\n");
    return;
  }
  printf(" -- swap_memory\n");
  printf("Total: %" PRIu64 "\n", r.total);
  printf("Used: %" PRIu64 "\n", r.used);
  printf("Free: %" PRIu64 "\n", r.free);
  printf("Percent: %f\n", r.percent);
  printf("Sin: %" PRIu64 "\n", r.sin);
  printf("Sout: %" PRIu64 "\n", r.sout);
  printf("\n");
}

void test_cpu_times() {
  CpuTimes *r;
  r = cpu_times(false);
  if (!r) {
    printf("Aborting\n");
    return;
  }
  printf(" -- cpu_times\n");
  printf("User: %.3lf;", r->user);
  printf(" Nice: %.3lf;", r->nice);
  printf(" System: %.3lf;", r->system);
  printf(" Idle: %.3lf;", r->idle);
  printf(" IOWait: %.3lf;", r->iowait);
  printf(" IRQ: %.3lf;", r->irq);
  printf(" SoftIRQ: %.3lf;", r->softirq);
  printf(" Steal: %.3lf;", r->steal);
  printf(" Guest: %.3lf;", r->guest);
  printf(" Guest nice: %.3lf\n", r->guest_nice);
  printf("\n\n");

  free(r);
}

void test_cpu_times_percpu() {
  CpuTimes *r, *c;
  uint32_t ncpus = cpu_count(true);
  c = r = cpu_times(true);
  if (!r) {
    printf("Aborting\n");
    return;
  }
  printf(" -- cpu_times_percpu\n");
  for (uint32_t i = 0; i < ncpus; i++) {
    printf("CPU %d :: ", i + 1);
    printf(" Usr: %.3lf;", c->user);
    printf(" Nice: %.3lf;", c->nice);
    printf(" Sys: %.3lf;", c->system);
    printf(" Idle: %.3lf;", c->idle);
    printf(" IOWait: %.3lf;", c->iowait);
    printf(" IRQ: %.3lf;", c->irq);
    printf(" SoftIRQ: %.3lf;", c->softirq);
    printf(" Steal: %.3lf;", c->steal);
    printf(" Guest: %.3lf;", c->guest);
    printf(" Guest nice: %.3lf\n", c->guest_nice);
    printf("\n");
    c++;
  }
  printf("\n");
  free(r);
}

void test_cpu_util_percent() {
  CpuTimes *info;
  double *utilisation;
  info = cpu_times(false);

  if (!info) {
    printf("Aborting\n");
    return;
  }

  usleep(100000);
  utilisation = cpu_util_percent(false, info);
  printf(" -- cpu_util_percent\n");
  printf("%f\n", *utilisation);
  printf("\n");

  free(utilisation);
  free(info);
}

void test_cpu_util_percent_percpu() {
  CpuTimes *info;
  double *percentages;
  uint32_t ncpus = cpu_count(true);
  info = cpu_times(1);

  if (!info) {
    printf("Aborting\n");
    return;
  }

  usleep(100000);
  percentages = cpu_util_percent(1, info);
  printf(" -- cpu_util_percent_percpu\n");
  for (uint32_t i = 0; i < ncpus; i++) {
    printf("Cpu #%d : %f\n", i, percentages[i]);
  }

  printf("\n");

  free(percentages);
  free(info);
}

void test_cpu_times_percent() {
  CpuTimes *info;
  CpuTimes *ret;
  info = cpu_times(false);
  if (!info) {
    printf("Aborting\n");
    return;
  }
  usleep(100000);
  ret = cpu_times_percent(false, info); /* Actual percentages since last call */
  if (!ret) {
    printf("Error while computing utilisation percentage\n");
    return;
  }
  printf(" -- cpu_times_percent\n");
  printf("CPU times as percentage of total (0.1 second sample)\n");
  printf("Usr: %.3lf;", ret->user);
  printf(" Nice: %.3lf;", ret->nice);
  printf(" Sys: %.3lf;", ret->system);
  printf(" Idle: %.3lf;", ret->idle);
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

void test_cpu_times_percent_percpu() {
  CpuTimes *info, *last, *r;
  uint32_t ncpus = cpu_count(true);
  last = cpu_times(true);
  if (!last) {
    printf("Aborting\n");
    return;
  }
  usleep(100000);
  r = info =
      cpu_times_percent(true, last); /* Actual percentages since last call */
  if (!info) {
    printf("Error while computing utilisation percentage\n");
    return;
  }

  printf(" -- cpu_times_percent_percpu\n");
  printf("CPU times as percentage of total per CPU (0.1 second sample)\n");
  for (uint32_t i = 0; i < ncpus; i++) {
    printf("CPU %d :: ", i + 1);
    printf("Usr: %.3lf;", info->user);
    printf(" Nice: %.3lf;", info->nice);
    printf(" Sys: %.3lf;", info->system);
    printf(" Idle: %.3lf;", info->idle);
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

void test_cpu_count() {
  uint32_t logical;
  uint32_t physical;
  logical = cpu_count(true);
  physical = cpu_count(false);
  printf(" -- cpu_count \n");
  if (logical == UINT32_MAX || physical == UINT32_MAX) {
    printf("Aborting\n");
    return;
  }
  printf("Logical : %d\nPhysical : %d\n", logical, physical);
  printf("\n");
}

void test_pid_exists() {
  pid_t pid = getpid();
  printf(" -- pid_exists \n");
  if (pid_exists(pid))
    printf("pid %d exists\n", pid);
  else {
    printf("pid %d does not exist\n", pid);
  }
  printf("\n");
}

void test_process() {
  pid_t pid = getpid();
  Process *process = get_process(pid);
  printf(" -- process \n");
  printf("pid %d\n", process->pid);
  printf("ppid %d\n", process->ppid);
  printf("name %s\n", process->name);
  printf("exe %s\n", process->exe);
  printf("cmdline %s\n", process->cmdline);
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

int main(void) {
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
  test_pid_exists();
  test_process();
}
