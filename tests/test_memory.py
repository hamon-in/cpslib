import pytest

import psutil
from pycpslib import lib as P
from pycpslib import ffi

@pytest.mark.linux2
def test_virtual_memory_linux(almost_equal, flush):
    pslib_vmem = ffi.new("VmemInfo *")
    P.virtual_memory(pslib_vmem)
    psutil_vmem = psutil.virtual_memory()

    assert almost_equal(pslib_vmem.total, psutil_vmem.total)
    assert almost_equal(pslib_vmem.available, psutil_vmem.available)
    assert almost_equal(pslib_vmem.used, psutil_vmem.used)
    assert almost_equal(pslib_vmem.free, psutil_vmem.free)
    assert almost_equal(pslib_vmem.active, psutil_vmem.active)
    assert almost_equal(pslib_vmem.inactive, psutil_vmem.inactive)
    assert almost_equal(pslib_vmem.buffers, psutil_vmem.buffers)
    assert almost_equal(pslib_vmem.cached, psutil_vmem.cached)
    assert almost_equal(pslib_vmem.percent, psutil_vmem.percent)

@pytest.mark.darwin
def test_virtual_memory_darwin(almost_equal, flush):
    pslib_vmem = ffi.new("VmemInfo *")
    P.virtual_memory(pslib_vmem)
    psutil_vmem = psutil.virtual_memory()

    assert almost_equal(pslib_vmem.total, psutil_vmem.total)
    assert almost_equal(pslib_vmem.free, psutil_vmem.free)
    assert almost_equal(pslib_vmem.active, psutil_vmem.active)
    assert almost_equal(pslib_vmem.inactive, psutil_vmem.inactive)
    assert almost_equal(pslib_vmem.wired, psutil_vmem.wired)
    assert almost_equal(pslib_vmem.available, psutil_vmem.available)
    assert almost_equal(pslib_vmem.percent, psutil_vmem.percent)
    assert almost_equal(pslib_vmem.used, psutil_vmem.used)

def test_swap(almost_equal, flush):
    pslib_swap = ffi.new("SwapMemInfo *")
    P.swap_memory(pslib_swap)
    psutil_vmem = psutil.swap_memory()

    assert almost_equal(pslib_swap.total, psutil_vmem.total)
    assert almost_equal(pslib_swap.used, psutil_vmem.used)
    assert almost_equal(pslib_swap.free, psutil_vmem.free)
    assert almost_equal(pslib_swap.percent, psutil_vmem.percent)
    assert almost_equal(pslib_swap.sin, psutil_vmem.sin)
    assert almost_equal(pslib_swap.sout, psutil_vmem.sout)
