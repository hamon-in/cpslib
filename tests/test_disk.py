import psutil
from pycpslib import lib as P
from pycpslib import ffi

def test_number_of_partitions(flush):
    expected_all_partitions = psutil.disk_partitions(True)  # All partitions
    expected_phy_partitions = psutil.disk_partitions(False) # Physical only

    actual_all_partitions = P.disk_partitions(0)
    actual_phy_partitions = P.disk_partitions(1)

    assert actual_all_partitions.nitems == len(expected_all_partitions)
    assert actual_phy_partitions.nitems == len(expected_phy_partitions)

def test_all_partition_attribs(flush):
    "Verifies device, mountpoint, fstype and opts for all partitions"
    psutil_partitions = psutil.disk_partitions(True)
    pslib_partitions = P.disk_partitions(0)

    for i in range(pslib_partitions.nitems):
        device = ffi.string(pslib_partitions.partitions[i].device)
        mountpoint = ffi.string(pslib_partitions.partitions[i].mountpoint)
        fstype = ffi.string(pslib_partitions.partitions[i].fstype)
        opts = ffi.string(pslib_partitions.partitions[i].opts)

        found = False
        for part in psutil_partitions:
            if all([part.mountpoint == mountpoint,
                    part.device == device,
                    part.fstype == fstype,
                    part.opts == opts]):
                found = True
        assert found, """No match for Partition(mountpoint = '{}', device = '{}', fstype = '{}', opts = '{}')""".format(mountpoint, device, fstype, opts)

def test_disk_usage(flush):
    for mountpoint in ["/", "/etc/", "/home", "/var"]:
        pslib_usage = ffi.new("DiskUsage *")
        P.disk_usage(mountpoint, pslib_usage)
        psutil_usage = psutil.disk_usage(mountpoint)
        assert psutil_usage.total == pslib_usage.total
        assert psutil_usage.used == pslib_usage.used
        assert psutil_usage.free == pslib_usage.free

def test_disk_io_counters(flush):
    psutil_counters = psutil.disk_io_counters(True)
    pslib_counter_info = P.disk_io_counters()

    for p in range(pslib_counter_info.nitems):
        name = ffi.string(pslib_counter_info.iocounters[p].name)
        readbytes = pslib_counter_info.iocounters[p].readbytes
        writebytes = pslib_counter_info.iocounters[p].writebytes
        reads = pslib_counter_info.iocounters[p].reads
        writes = pslib_counter_info.iocounters[p].writes
        readtime = pslib_counter_info.iocounters[p].readtime
        writetime = pslib_counter_info.iocounters[p].writetime

        assert psutil_counters[name].read_bytes == readbytes
        assert psutil_counters[name].read_count == reads
        assert psutil_counters[name].read_time == readtime
        assert psutil_counters[name].write_bytes == writebytes
        assert psutil_counters[name].write_count == writes
        assert psutil_counters[name].write_time == writetime
