import psutil
from pycpslib import lib as P 
from pycpslib import ffi

def test_boot_time(flush):
    pslib_boot_time = P.get_boot_time()
    psutil_boot_time = psutil.boot_time()
    
    assert pslib_boot_time == psutil_boot_time

def test_cpu_times_total(almost_equal, flush):
    pslib_cputimes = P.cpu_times(0) 
    psutil_cputimes = psutil.cpu_times(False)
    
    assert almost_equal(pslib_cputimes.user, psutil_cputimes.user)
    assert almost_equal(pslib_cputimes.system, psutil_cputimes.system)
    assert almost_equal(pslib_cputimes.idle, psutil_cputimes.idle)
    assert almost_equal(pslib_cputimes.nice, psutil_cputimes.nice)
    assert almost_equal(pslib_cputimes.iowait, psutil_cputimes.iowait)
    assert almost_equal(pslib_cputimes.irq, psutil_cputimes.irq)
    assert almost_equal(pslib_cputimes.softirq, psutil_cputimes.softirq)
    assert almost_equal(pslib_cputimes.steal, psutil_cputimes.steal)
    assert almost_equal(pslib_cputimes.guest, psutil_cputimes.guest)
    assert almost_equal(pslib_cputimes.guest_nice, psutil_cputimes.guest_nice)

def test_cpu_times_individual(almost_equal, flush):
    pslib_cputimes = P.cpu_times(1) 
    psutil_cputimes = psutil.cpu_times(True)

    for i,psutil_measure in enumerate(psutil_cputimes):
        pslib_measure = pslib_cputimes[i]
        assert almost_equal(pslib_measure.user, psutil_measure.user)
        assert almost_equal(pslib_measure.system, psutil_measure.system)
        assert almost_equal(pslib_measure.idle, psutil_measure.idle)
        assert almost_equal(pslib_measure.nice, psutil_measure.nice)
        assert almost_equal(pslib_measure.iowait, psutil_measure.iowait)
        assert almost_equal(pslib_measure.irq, psutil_measure.irq)
        assert almost_equal(pslib_measure.softirq, psutil_measure.softirq)
        assert almost_equal(pslib_measure.steal, psutil_measure.steal)
        assert almost_equal(pslib_measure.guest, psutil_measure.guest)
        assert almost_equal(pslib_measure.guest_nice, psutil_measure.guest_nice)
        
def test_cpu_count(almost_equal, flush):
    assert P.cpu_count(1) == psutil.cpu_count(True), "Mismatch in number of logical CPUs"
    assert P.cpu_count(0) == psutil.cpu_count(False), "Mismatch in number of physical CPUs"
