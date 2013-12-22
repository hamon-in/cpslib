CFLAGS=-g -Wall 


driver: driver.c pslib_linux.o common.o
	gcc ${CFLAGS} driver.c pslib_linux.o common.o -lm -o driver

pslib_linux.o:  pslib_linux.c pslib_linux.h
	gcc ${CFLAGS} -c pslib_linux.c -lm -o pslib_linux.o

common.o: common.c common.h
	gcc ${CFLAGS}  -O3 -c common.c -lm -o common.o

clean:
	rm -f *.o a.out

check-syntax:
	gcc -Wall -o nul -S ${CHK_SOURCES}

