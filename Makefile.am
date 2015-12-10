if OS_DARWIN
platformsrc = pslib_osx.c
endif

if OS_LINUX
platformsrc = pslib_linux.c
endif

bin_PROGRAMS = driver
driver_SOURCES = common.c driver.c $(platformsrc)

AM_CFLAGS = -Wall -Werror -Wunused -Wextra -std=gnu11
AUTOMAKE_OPTIONS = foreign
