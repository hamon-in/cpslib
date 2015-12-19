import psutil
from pycpslib import lib as P 
from pycpslib import ffi

def test_boot_time():
    pslib_boot_time = P.get_boot_time()
    psutil_boot_time = psutil.boot_time()
    
    assert pslib_boot_time == psutil_boot_time
