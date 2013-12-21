#ifndef __pslib_linux_h
#define __pslib_linux_h

typedef struct  {
  unsigned long totalram;
  unsigned long freeram;
  unsigned long bufferram;
  unsigned long sharedram;
  unsigned long totalswap;
  unsigned long freeswap;
} Pslib_Sysinfo;

Pslib_Sysinfo* get_sysinfo();

#endif
