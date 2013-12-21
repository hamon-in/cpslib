driver: driver.c pslib_linux.o
	gcc -Wall -O3 driver.c pslib_linux.o 

pslib_linux.o:  pslib_linux.c pslib_linux.h
	gcc -Wall -O3 -c pslib_linux.c -o pslib_linux.o
