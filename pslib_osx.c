#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/sysctl.h>
#include <utmpx.h>

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


UsersInfo *
get_users ()
{
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
    check_mem(u->username);

    u->hostname = strdup(utx->ut_host);
    check_mem(u->hostname);

    u->tstamp = utx->ut_tv.tv_sec;

    ret->nitems++;
    u++;

    if (ret->nitems == nusers) { /* More users than we've allocated space for. */
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

void
free_users_info(UsersInfo * ui)
{
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
