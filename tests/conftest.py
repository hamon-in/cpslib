import pytest
import sys

from pycpslib import lib as P

ALL = set("darwin linux2 win32".split())

def pytest_runtest_setup(item):
    if isinstance(item, item.Function):
        plat = sys.platform
        if not item.get_marker(plat):
            if ALL.intersection(item.keywords):
                pytest.skip("cannot run on platform %s" %(plat))

@pytest.fixture
def flush(request):
    "Flushes gcov data onto disk after test is executed."
    def gcov_flush():
        P.gcov_flush()
    request.addfinalizer(gcov_flush)

@pytest.fixture
def almost_equal():
    # This function is taken from the unittest.case module.

    def almost_equal(first, second, places=None, delta=0.1):
        """Fail if the two objects are unequal as determined by their
           difference rounded to the given number of decimal places
           (default 7) and comparing to zero, or by comparing that the
           between the two objects is more than the given delta.

           Note that decimal places (from zero) are usually not the same
           as significant digits (measured from the most signficant digit).

           If the two objects compare equal then they will automatically
           compare almost equal.
        """
        if first == second:
            # shortcut
            return True
        if delta is not None and places is not None:
            raise TypeError("specify delta or places not both")
        if delta is not None:
            if abs(first - second) <= delta:
                return True
        else:
            if places is None:
                places = 2

            if round(abs(second-first), places) == 0:
                return True
        return False

    return almost_equal

@pytest.fixture
def compare_cpu_times(almost_equal):
    def compare_linux(t1, t2):
        return all([almost_equal(t1.user, t2.user),
                    almost_equal(t1.system, t2.system),
                    almost_equal(t1.idle, t2.idle),
                    almost_equal(t1.nice, t2.nice),
                    almost_equal(t1.iowait, t2.iowait),
                    almost_equal(t1.irq, t2.irq),
                    almost_equal(t1.softirq, t2.softirq),
                    almost_equal(t1.steal, t2.steal),
                    almost_equal(t1.guest, t2.guest),
                    almost_equal(t1.guest_nice, t2.guest_nice)])

    def compare_darwin(t1, t2):
        return all([almost_equal(t1.user, t2.user),
                    almost_equal(t1.system, t2.system),
                    almost_equal(t1.idle, t2.idle),
                    almost_equal(t1.nice, t2.nice)])

    if sys.platform == 'darwin':
        return compare_darwin
    # TODO: add more cases as more platforms get implemented
    # See https://github.com/nibrahim/cpslib/issues/9 as well.

    return compare_linux
