from _libps_abi_out_of_line import ffi

lib = ffi.dlopen("libpslib.so")

logical = lib.cpu_count(1)
physical = lib.cpu_count(0)

print "Logical {}, Physical {}".format(logical, physical)

