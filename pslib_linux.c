#include <stdlib.h>
#include <sys/sysinfo.h>
#include "pslib_linux.h"

Pslib_Sysinfo*
get_sysinfo()
{
  struct sysinfo info;
  Pslib_Sysinfo *pslib_sysinfo = (Pslib_Sysinfo*)malloc(sizeof(Pslib_Sysinfo));
  if (sysinfo(&info) != 0) {
    /* Error out over here */
    ;
  }
  pslib_sysinfo->totalram  = info.totalram  * info.mem_unit;
  pslib_sysinfo->freeram   = info.freeram   * info.mem_unit;
  pslib_sysinfo->bufferram = info.bufferram * info.mem_unit;
  pslib_sysinfo->sharedram = info.sharedram * info.mem_unit;
  pslib_sysinfo->totalswap = info.totalswap * info.mem_unit;
  pslib_sysinfo->freeswap  = info.freeswap  * info.mem_unit;

  return pslib_sysinfo;
}

