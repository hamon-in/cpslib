# pycpslib.py

import os
import sys
from cffi import FFI

ffi = FFI()
project_root = os.path.abspath("../../")
if sys.platform == "win32":
    ffi.set_source("pycpslib",
                """#include "pslib.h"	   
                """,
                sources=["../../pslib_windows.c", "../../common.c"],
                library_dirs = [project_root],
                include_dirs = [project_root])

    ffi.cdef('''
    typedef int32_t pid_t;
    typedef int32_t bool;
    #define BELOW_NORMAL_PRIORITY_CLASS       0x00004000
    #define ABOVE_NORMAL_PRIORITY_CLASS       0x00008000
    #define NORMAL_PRIORITY_CLASS             0x00000020
    #define IDLE_PRIORITY_CLASS               0x00000040
    #define HIGH_PRIORITY_CLASS               0x00000080
    #define REALTIME_PRIORITY_CLASS           0x00000100
    ''',override = True)

    lines = open(project_root+"/pslib.h").readlines()
    altered_lines = []
    flag = 1
    for line in lines:
	if line.startswith('#include') or line.startswith('#define'):
            altered_lines.append("")
	elif line.startswith('#ifdef _WIN32'):
            flag = 2
	elif line.startswith('#else') :
            flag = 0
	elif line.startswith('#endif'):
            flag = 1
	else:
            if (flag != 0):
                altered_lines.append(line)
    ffi.cdef(''.join(altered_lines),override = True)
    
else:
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
