import psutil
from pycpslib import lib as P 
from pycpslib import ffi

def test_boot_time(flush):
    pslib_boot_time = P.get_boot_time()
    psutil_boot_time = psutil.boot_time()
    
    assert pslib_boot_time == psutil_boot_time

def test_cpu_times_total(compare_cpu_times, flush):
    pslib_cputimes = P.cpu_times(0) 
    psutil_cputimes = psutil.cpu_times(False)
    assert compare_cpu_times(pslib_cputimes, psutil_cputimes)

def test_cpu_times_individual(compare_cpu_times, flush):
    pslib_cputimes = P.cpu_times(1) 
    psutil_cputimes = psutil.cpu_times(True)

    for i,psutil_measure in enumerate(psutil_cputimes):
        pslib_measure = pslib_cputimes[i]
        assert compare_cpu_times(pslib_measure, psutil_measure)
        
def test_cpu_count(almost_equal, flush):
    assert P.cpu_count(1) == psutil.cpu_count(True), "Mismatch in number of logical CPUs"
    assert P.cpu_count(0) == psutil.cpu_count(False), "Mismatch in number of physical CPUs"

def test_cpu_times_percent(compare_cpu_times, flush):
    t1 = P.cpu_times(0)
    # We need a delay of at least a second here to get close readings for both the APIs
    psutil_times_percent = psutil.cpu_times_percent(1)
    pslib_times_percent = P.cpu_times_percent(0, t1)
    compare_cpu_times(psutil_times_percent, pslib_times_percent)

def test_cpu_times_percent_per_cpu(compare_cpu_times, flush):
    t1 = P.cpu_times(1)
    # We need a delay of at least a second here to get close readings for both the APIs
    psutil_times_percent = psutil.cpu_times_percent(1, True)
    pslib_times_percent = P.cpu_times_percent(1, t1)

    for i,psutil_measure in enumerate(psutil_times_percent):
        pslib_measure = pslib_times_percent[i]
        assert compare_cpu_times(pslib_measure, psutil_measure)

