import psutil
from pycpslib import lib as P 
from pycpslib import ffi

def test_virtual_memory():
    pslib_vmem = ffi.new("VmemInfo *")
    P.virtual_memory(pslib_vmem)

    psutil_vmem = psutil.virtual_memory()
    
    assert pslib_vmem.total == psutil_vmem.total
    assert pslib_vmem.available == psutil_vmem.available
    assert pslib_vmem.used == psutil_vmem.used
    assert pslib_vmem.free == psutil_vmem.free
    assert pslib_vmem.active == psutil_vmem.active
    assert pslib_vmem.inactive == psutil_vmem.inactive
    assert pslib_vmem.buffers == psutil_vmem.buffers
    assert pslib_vmem.cached == psutil_vmem.cached
    assert pslib_vmem.percent == psutil_vmem.percent
