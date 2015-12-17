from cffi import FFI

ffi = FFI()

ffi.cdef("""
int cpu_count(int);
""")

lib = ffi.dlopen("libpslib.so")

logical = lib.cpu_count(1)
physical = lib.cpu_count(0)

print "Logical {}, Physical {}".format(logical, physical)

         

