# Introduction
cpslib is an attempt to port the excellent Python library [psutil](https://code.google.com/p/psutil/) to C and then write wrappers for it in other languages (like Rust).

# Design
There are 3 platform specific files which implement all the API calls mentioned in the documentation. They will hide the details of the actual OS level function which is called to get the job done.
The top level header file `cpslib.h` has all the constants and structures necessary provide the cross platform API.


