import psutil
from pycpslib import lib as P
from pycpslib import ffi

def test_net_io_counters(flush):
    psutil_counters = psutil.net_io_counters(True)
    pslib_counter_info = P.net_io_counters()

    for p in range(pslib_counter_info.nitems):
        name = ffi.string(pslib_counter_info.iocounters[p].name)
        bytes_sent = pslib_counter_info.iocounters[p].bytes_sent
        bytes_recv = pslib_counter_info.iocounters[p].bytes_recv
        packets_sent = pslib_counter_info.iocounters[p].packets_sent
        packets_recv = pslib_counter_info.iocounters[p].packets_recv
        errin = pslib_counter_info.iocounters[p].errin
        errout = pslib_counter_info.iocounters[p].errout
        dropin = pslib_counter_info.iocounters[p].dropin
        dropout = pslib_counter_info.iocounters[p].dropout

        assert psutil_counters[name].bytes_sent == bytes_sent
        assert psutil_counters[name].bytes_recv == bytes_recv
        assert psutil_counters[name].packets_sent == packets_sent
        assert psutil_counters[name].packets_recv == packets_recv
        assert psutil_counters[name].errin == errin
        assert psutil_counters[name].errout == errout
        assert psutil_counters[name].dropin == dropin
        assert psutil_counters[name].dropout == dropout
