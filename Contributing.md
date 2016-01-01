# Contributing to cpslib

Thank you for contributing to cpslib! This file describes a few
guidelines I try to follow while developing the project. It would be
very useful to me if you could do so too so that I can merge your
commits without any unnecessary extra effort.

# General workflow

The general workflow to is fork the repository, clone it, cut a branch
with a name that describes the topic (e.g. `docfix`, `issue42`,
`diskio-windows` etc.) and make your commits there. Please don't make
commits on `master`. It makes it harder for me to keep track. Push
these changes back to your fork and send me a pull request.

# Driver code

When you add new functionality, say `get_system_stats`, please include
a call to this function inside `driver.c` (preferably called
`test_get_system_stats`) that calls `get_system_stats`, prints out all
the data in a human readable way and then frees all resources
allocated. I'm hoping that the `driver.c` code will serve as examples
that can be cut and pasted into actual programs that people
use. Running `driver` should give display all the information that
cpslib is able to gather about the system.

# Add test cases

The `bindings/python` directory has the `pycpslib_build.py` script
which can be used to create `cffi` based bindings for the
library. Steps:

1. Execute `make shared` from the project root to build pslib dynamic
   library `libpslib.so` (`libpslib.dylib` for osx), then export your `LD_LIBRARY_PATH` (`DYLD_LIBRARY_PATH` for osx) to include
   project root (`export LD_LIBRARY_PATH=$(pwd)`) so
   that the shared library can get picked up.
1. Navigate to `bindings/python` and change the build script as
   necessary when your new function has
   been added and then run `python setup.py develop` to install a dev
   version of the bindings into your virtualenv
1. Add a test to the `tests` directory in a new file or existing file
   as appropriate.
1. Install requirements to run tests. They're mentioned in
   `tests/requirements.txt` so `pip install -r tests/requirements.txt`
   should do it.
1. Run the tests using `py.test tests` and they should all work fine.

This process of running will be automated but it's that's not done yet.

# Commits

Please make sure that the the commits you make on every branch are
topical (e.g. don't create a branch called `formatting-fix` and then
include commits there which add features or fix bugs).

Please make sure that the commits are small and well
contained. Commits that implement more than one piece of functionality
should be broken down into multiple commits that implement one
each. This helps keep the history readable and manageable. As a
concrete example, please don't create commits which "implements
swap_counts, disk_counters and cpu_info". Split it into three that do
one each.

# Valgrind

After you implement your feature and create a call in the driver code,
please run `driver` with valgrind to see if there are any memory
leaks. Please make sure there are none before sending me a pull
request.

That's it for now. As the project matures and breaks up into multiple
files, I will update this document.

Thanks again!
