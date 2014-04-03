CFLAGS=-g -Wall 

ifdef COMSPEC
	PLATFORM := "WINDOWS"
else
	PLATFORM := "LINUX"
endif

driver: driver.c pslib.o pslib_linux.o common.o
	gcc -D${PLATFORM} ${CFLAGS} driver.c pslib.o pslib_linux.o common.o -lm -o driver

pslib_linux.o:  pslib_linux.c pslib.h
	gcc -D${PLATFORM}  ${CFLAGS} -c pslib_linux.c -lm -o pslib_linux.o

common.o: common.c common.h
	gcc -D${PLATFORM}  ${CFLAGS}  -c common.c -lm -o common.o

pslib.o: pslib.h pslib.c
	gcc -D${PLATFORM}  ${CFLAGS} -c pslib.c -lm -o pslib.o

clean:
	rm -f *.o a.out driver

check-syntax:
	gcc -Wall -o nul -S ${CHK_SOURCES}

