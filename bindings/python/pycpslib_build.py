# pycpslib.py

import os

from cffi import FFI

ffi = FFI()
project_root = os.path.abspath("../../")

ffi.set_source("pycpslib",
               """#include <stdio.h>
               #include <stdlib.h>
               #include <sys/types.h>
               #include <unistd.h>
               #include "pslib.h"
               """,
               libraries = ["pslib"],
               library_dirs = [project_root],
               include_dirs = [project_root])

ffi.cdef('''
typedef int32_t pid_t;
typedef int32_t bool;
''')

lines = open(project_root+"/pslib.h").readlines()

altered_lines = ['' if line.startswith('#include ') else line for line in lines]

ffi.cdef(''.join(altered_lines))

if __name__ == '__main__':
    ffi.compile()
