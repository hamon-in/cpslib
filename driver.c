#include <stdio.h>
#include "pslib_linux.h"



int
main()
{
  Pslib_Sysinfo *st;
  st = get_sysinfo();
  printf("Total ram : %ld\n", st->totalram);
  return 0;
}
