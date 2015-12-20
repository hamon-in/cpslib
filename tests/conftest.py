import pytest

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
