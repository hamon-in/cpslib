if OS_DARWIN
platformsrc = pslib_osx.c
endif

if OS_LINUX
platformsrc = pslib_linux.c
endif

bin_PROGRAMS = driver
driver_SOURCES = common.c driver.c $(platformsrc)

AUTOMAKE_OPTIONS = foreign
