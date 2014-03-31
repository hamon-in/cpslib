# Introduction
cpslib is an attempt to port the excellent Python library [psutil](https://code.google.com/p/psutil/) to C and then write wrappers for it in other languages (like Rust).

# Design
There are 3 platform specific files which implement all the API calls mentioned in the documentation. They will hide the details of the actual OS level function which is called to get the job done.
The top level header file `cpslib.h` has all the constants and structures necessary provide the cross platform API.

# Open questions
 * The Process structure contains information which will change after it is initialised. It might be a good idea to keep the fields inside the structure minimal and provide functions which will retrieve various attributes on demand.
 * I'm unsure about the types I've used in the wrapper functions. I've indiscriminately used basic numeric types for various system parameters like process id etc. 


