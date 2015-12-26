import psutil
from pycpslib import lib as P 
from pycpslib import ffi

def test_boot_time(flush):
    pslib_boot_time = P.get_boot_time()
    psutil_boot_time = psutil.boot_time()
    
    assert pslib_boot_time == psutil_boot_time

def test_cpu_times(almost_equal, flush):
    pslib_accumulated_cputimes = P.cpu_times(0) 
    psutil_accumulated_cputimes = psutil.cpu_times(False)
    
    assert almost_equal(pslib_accumulated_cputimes.user, psutil_accumulated_cputimes.user)
    assert almost_equal(pslib_accumulated_cputimes.system, psutil_accumulated_cputimes.system)
    assert almost_equal(pslib_accumulated_cputimes.idle, psutil_accumulated_cputimes.idle)
    assert almost_equal(pslib_accumulated_cputimes.nice, psutil_accumulated_cputimes.nice)
    assert almost_equal(pslib_accumulated_cputimes.iowait, psutil_accumulated_cputimes.iowait)
    assert almost_equal(pslib_accumulated_cputimes.irq, psutil_accumulated_cputimes.irq)
    assert almost_equal(pslib_accumulated_cputimes.softirq, psutil_accumulated_cputimes.softirq)
    assert almost_equal(pslib_accumulated_cputimes.steal, psutil_accumulated_cputimes.steal)
    assert almost_equal(pslib_accumulated_cputimes.guest, psutil_accumulated_cputimes.guest)
    assert almost_equal(pslib_accumulated_cputimes.guest_nice, psutil_accumulated_cputimes.guest_nice)

    # fields = ['user', 'nice', 'system', 'idle', 'iowait', 'irq', 'softirq']
    # vlen = len(values)
    # if vlen >= 8:
    #     # Linux >= 2.6.11
    #     fields.append('steal')
    # if vlen >= 9:
    #     # Linux >= 2.6.24
    #     fields.append('guest')
    # if vlen >= 10:
    #     # Linux >= 3.2.0
    #     fields.append('guest_nice')
