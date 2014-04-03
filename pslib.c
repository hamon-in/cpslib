#include <stdlib.h>

#include "pslib.h"

DiskIOCounters* disk_io_counter() {
  DiskIOCounterInfo *info = disk_io_counters();
  DiskIOCounters *ret = calloc(1, sizeof(DiskIOCounters));

  int i;
  for(i=0;i < info->nitems; i++) {
    DiskIOCounters *cur = info->iocounters + i;
    ret->readbytes += cur->readbytes;
    ret->writebytes += cur->writebytes;
    ret->reads += cur->reads;
    ret->writes += cur->writes;
    ret->readtime += cur->readtime;
    ret->writetime += cur->writetime;
  }

  free_disk_iocounter_info(info);
  return ret;
}

NetIOCounters *net_io_counter() {
  NetIOCounterInfo *info = net_io_counters();
  NetIOCounters *ret = calloc(1, sizeof(NetIOCounters));

  int i;
  for(i=0;i < info->nitems; i++) {
    NetIOCounters *cur = info->iocounters + i;
    ret->bytes_sent += cur->bytes_sent;
    ret->bytes_recv += cur->bytes_recv;
    ret->packets_sent += cur->packets_sent;
    ret->packets_recv += cur->packets_recv;
    ret->errin += cur->errin;
    ret->errout += cur->errout;
    ret->dropin += cur->dropin;
    ret->dropout += cur->dropout;
  }
  free_net_iocounter_info(info);
  return ret;
}
