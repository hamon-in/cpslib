#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/sysctl.h>
#include <utmpx.h>
#include <mach/mach.h>

#include "pslib.h"
#include "common.h"

/* TBD : Generic function to get field from a line in a file that starts with
 * something */

/* Internal functions */
CpuTimes *per_cpu_times();

/* Public functions */

float get_boot_time() {
  /* read KERN_BOOTIME */
  int mib[2] = {CTL_KERN, KERN_BOOTTIME};
  struct timeval result;
  size_t len = sizeof result;
  time_t boot_time = 0;

  check(sysctl(mib, 2, &result, &len, NULL, 0) != -1, "sysctl failed");
  boot_time = result.tv_sec;
  return (float)boot_time;

error:
  return -1;
}

int cpu_count(int logical) {
  int ncpu;
  size_t len = sizeof(ncpu);

  if (logical) {
    check(sysctlbyname("hw.logicalcpu", &ncpu, &len, NULL, 0) != -1,
          "sysctl failed");
  } else {
    check(sysctlbyname("hw.physicalcpu", &ncpu, &len, NULL, 0) != -1,
          "sysctl failed");
  }

  return ncpu;

error:
  return -1;
}

CpuTimes *cpu_times(int percpu) {
  CpuTimes *ret = NULL;

  if (!percpu) {
    ret = (CpuTimes *)calloc(1, sizeof(CpuTimes));

    mach_msg_type_number_t count = HOST_CPU_LOAD_INFO_COUNT;
    kern_return_t kerror;
    host_cpu_load_info_data_t r_load;

    mach_port_t host_port = mach_host_self();
    kerror = host_statistics(host_port, HOST_CPU_LOAD_INFO,
                             (host_info_t)&r_load, &count);
    check(kerror == KERN_SUCCESS, "Error in host_statistics(): %s",
          mach_error_string(kerror));
    mach_port_deallocate(mach_task_self(), host_port);

    ret->user = (double)r_load.cpu_ticks[CPU_STATE_USER] / CLK_TCK;
    ret->nice = (double)r_load.cpu_ticks[CPU_STATE_NICE] / CLK_TCK;
    ret->system = (double)r_load.cpu_ticks[CPU_STATE_SYSTEM] / CLK_TCK;
    ret->idle = (double)r_load.cpu_ticks[CPU_STATE_IDLE] / CLK_TCK;

    return ret;
  } else {
    return per_cpu_times();
  }

error:
  if (ret)
    free(ret);
  return NULL;
}

CpuTimes *per_cpu_times() {
  CpuTimes *ret = NULL;

  natural_t cpu_count;
  processor_info_array_t info_array;
  mach_msg_type_number_t info_count;
  kern_return_t kerror;
  processor_cpu_load_info_data_t *cpu_load_info = NULL;
  int sysret;

  mach_port_t host_port = mach_host_self();
  kerror = host_processor_info(host_port, PROCESSOR_CPU_LOAD_INFO, &cpu_count,
                               &info_array, &info_count);
  check(kerror == KERN_SUCCESS, "Error in host_processor_info(): %s",
        mach_error_string(kerror));
  mach_port_deallocate(mach_task_self(), host_port);

  cpu_load_info = (processor_cpu_load_info_data_t *)info_array;

  ret = (CpuTimes *)calloc(cpu_count, sizeof(CpuTimes));
  check_mem(ret);

  for (unsigned int i = 0; i < cpu_count; i++) {
    (ret + i)->user =
        (double)cpu_load_info[i].cpu_ticks[CPU_STATE_USER] / CLK_TCK;
    (ret + i)->nice =
        (double)cpu_load_info[i].cpu_ticks[CPU_STATE_NICE] / CLK_TCK;
    (ret + i)->system =
        (double)cpu_load_info[i].cpu_ticks[CPU_STATE_SYSTEM] / CLK_TCK;
    (ret + i)->idle =
        (double)cpu_load_info[i].cpu_ticks[CPU_STATE_IDLE] / CLK_TCK;
  }

  sysret = vm_deallocate(mach_task_self(), (vm_address_t)info_array,
                         info_count * sizeof(int));
  if (sysret != KERN_SUCCESS)
    log_warn("vm_deallocate() failed");

  return ret;

error:
  if (ret)
    free(ret);
  if (cpu_load_info != NULL) {
    sysret = vm_deallocate(mach_task_self(), (vm_address_t)info_array,
                           info_count * sizeof(int));
    if (sysret != KERN_SUCCESS)
      log_warn("vm_deallocate() failed");
  }
  return NULL;
}

UsersInfo *get_users() {
  int nusers = 100;

  UsersInfo *ret = (UsersInfo *)calloc(1, sizeof(UsersInfo));
  check_mem(ret);
  Users *users = (Users *)calloc(nusers, sizeof(Users));
  check_mem(users);
  Users *u = users;

  struct utmpx *utx;

  ret->nitems = 0;
  ret->users = users;

  while (NULL != (utx = getutxent())) {
    if (utx->ut_type != USER_PROCESS)
      continue;

    u->username = strdup(utx->ut_user);
    check_mem(u->username);

    u->tty = strdup(utx->ut_line);
    check_mem(u->tty);

    u->hostname = strdup(utx->ut_host);
    check_mem(u->hostname);

    u->tstamp = utx->ut_tv.tv_sec;

    ret->nitems++;
    u++;

    if (ret->nitems ==
        nusers) { /* More users than we've allocated space for. */
      nusers *= 2;
      users = realloc(users, sizeof(Users) * nusers);
      check_mem(users);
      ret->users = users;
      u = ret->users + ret->nitems; /* Move the cursor to the correct
                                       value in case the realloc moved
                                       the memory */
    }
  }
  endutxent();
  return ret;

error:
  free_users_info(ret);
  return NULL;
}

void free_users_info(UsersInfo *ui) {
  Users *u = ui->users;
  while (ui->nitems--) {
    free(u->username);
    free(u->tty);
    free(u->hostname);
    u++;
  }
  free(ui->users);
  free(ui);
}
