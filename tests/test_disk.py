import psutil
from pycpslib import lib as P


def test_number_of_partitions():
    expected_all_partitions = psutil.disk_partitions(True)  # All partitions
    expected_phy_partitions = psutil.disk_partitions(False) # Physical only
 
    actual_all_partitions = P.disk_partitions(0)
    actual_phy_partitions = P.disk_partitions(1)

    assert actual_all_partitions.nitems == len(expected_all_partitions)
    assert actual_phy_partitions.nitems == len(expected_phy_partitions)

