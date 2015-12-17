from cffi import FFI

ffi = FFI()
ffi.set_source("_libps_abi_out_of_line", None)
ffi.cdef("""
int cpu_count(int);
""")

if __name__ == '__main__':
    ffi.compile()
