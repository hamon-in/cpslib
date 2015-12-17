from cffi import FFI

ffi = FFI()


# gcc -DNDEBUG -g -O3 -Wall -Wstrict-prototypes -fPIC -DMAJOR_VERSION=1 -DMINOR_VERSION=0  -c _libps_api_out_of_line.c -o _libps_api_out_of_line.o

# gcc -shared build/temp.linux-i686-2.2/demo.o -L/usr/local/lib -ltcl83 -o build/lib.linux-i686-2.2/demo.so

ffi.set_source("_libps_api_out_of_line",
               """#include <stdio.h>
               #include <stdlib.h>
               #include <sys/types.h>
               #include <unistd.h>
               """,
               libraries = ["pslib"],
               library_dirs = ["."])


ffi.cdef("""
int cpu_count(int);
""")

if __name__ == '__main__':
    ffi.compile()
