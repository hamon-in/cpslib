# Introduction
cpslib is an attempt to port the excellent Python library [psutil](https://github.com/giampaolo/psutil) to C and then write wrappers for it in other languages (like Rust).

# Design
There are 3 platform specific files which implement all the API calls mentioned in the documentation. They will hide the details of the actual OS level function which is called to get the job done.
The top level header file `pslib.h` has all the constants and structures necessary to provide the cross platform API.

# Open questions
 * The Process structure contains information which will change after it is initialized. It might be a good idea to keep the fields inside the structure minimal and provide functions which will retrieve various attributes on demand.
 * I'm unsure about the types I've used in the wrapper functions. I've indiscriminately used basic numeric types for various system parameters like process id etc. 


# Status of implementation
## APIs available
This is a quick list of the APIs currently implemented. Proper
documentation will be done separately.

  * `disk_usage` - Information on total, used and free space on a partition
  * `disk_partitions` - Information on disk partitions on the system (device, mountpoint, type and options).
  * `disk_io_counters` - Information on disk I/O counters (number of reads/writes, bytes read/written, readtime, writetime).
  * `net_io_counters` - Information I/O counters on network interfaces (bytes/packets sent/received, input/output errors/drops).
  * `get_users` - List of users logged in and their information (username, tty, hostname and timestamp).
  * `get_boot_time` - Returns time of system boot.
  * `virtual_memory` - Returns Virtual memory information (used, free, available etc.)
  * `logical_cpu_count` - Returns number of logical CPUs.
  * `physical_cpu_count` - Returns number of physical CPUs.
  * `cpu_count` - Entry point for above functions.
  * `get_process` - Get detailed information about a process.

## Platforms supported
 * Linux - In progress
 * OS X - In progress
 * *BSD - Planned
 * Windows - Planned
