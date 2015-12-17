import psutil
from pycpslib import lib as P 
from pycpslib import ffi

def test_number_of_partitions():
    expected_all_partitions = psutil.disk_partitions(True)  # All partitions
    expected_phy_partitions = psutil.disk_partitions(False) # Physical only
 
    actual_all_partitions = P.disk_partitions(0)
    actual_phy_partitions = P.disk_partitions(1)

    assert actual_all_partitions.nitems == len(expected_all_partitions)
    assert actual_phy_partitions.nitems == len(expected_phy_partitions)


def test_all_parition_attribs():
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
