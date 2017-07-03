#pragma once

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>
#ifdef _WIN32
    #define pid_t uint32_t
#endif
enum proc_status {
  STATUS_RUNNING,
  STATUS_SLEEPING,
  STATUS_DISK_SLEEP,
  STATUS_STOPPED,
  STATUS_TRACING_STOP,
  STATUS_ZOMBIE,
  STATUS_DEAD,
  STATUS_WAKE_KILL,
  STATUS_WAKING,
  STATUS_IDLE,
  STATUS_LOCKED,
  STATUS_WAITING,
  STATUS_SUSPENDED
};

enum ioprio_class {
  IOPRIO_CLASS_NONE,
  IOPRIO_CLASS_RT,
  IOPRIO_CLASS_BE,
  IOPRIO_CLASS_IDLE
};

/*enum rlimit {
  RLIMIT_INFINITY,
  RLIMIT_AS,
  RLIMIT_CORE,
  RLIMIT_CPU,
  RLIMIT_DATA,
  RLIMIT_FSIZE,
  RLIMIT_LOCKS,
  RLIMIT_MEMLOCK,
  RLIMIT_MSGQUEUE,
  RLIMIT_NICE,
  RLIMIT_NOFILE,
  RLIMIT_NPROC,
  RLIMIT_RSS,
  RLIMIT_RTPRIO,
  RLIMIT_RTTIME,
  RLIMIT_SIGPENDING,
  RLIMIT_STACK
};*/

enum con_status {
  ESTABLISHED,
  SYN_SENT,
  SYN_RECV,
  FIN_WAIT1,
  FIN_WAIT2,
  TIME_WAIT,
  CLOSE,
  CLOSE_WAIT,
  LAST_ACK,
  LISTEN,
  CLOSING,
  NONE,
  DELETE_TCB,
  IDLE,
  BOUND
};

enum proc_priority {
	ABOVE_NORMAL_PRIORITY = ABOVE_NORMAL_PRIORITY_CLASS,
	BELOW_NORMAL_PRIORITY = BELOW_NORMAL_PRIORITY_CLASS,
	HIGH_PRIORITY = HIGH_PRIORITY_CLASS,
	IDLE_PRIORITY = IDLE_PRIORITY_CLASS,
	NORMAL_PRIORITY = NORMAL_PRIORITY_CLASS,
	REALTIME_PRIORITY = REALTIME_PRIORITY_CLASS,
	PRIORITY_ERROR
};

typedef struct {
  uint64_t total;
  uint64_t used;
  uint64_t free;
  float percent;
} DiskUsage;

typedef struct {
  char *device;
  char *mountpoint;
  char *fstype;
  char *opts;
} DiskPartition; /* TODO: Pluralise */

typedef struct {
  uint32_t nitems;
  DiskPartition *partitions;
} DiskPartitionInfo;

typedef struct {
  char *name;
  uint64_t readbytes;
  uint64_t writebytes;
  uint64_t reads;
  uint64_t writes;
  uint64_t readtime;
  uint64_t writetime;
} DiskIOCounters;

typedef struct {
  uint32_t nitems;
  DiskIOCounters *iocounters;
} DiskIOCounterInfo;

typedef struct {
  char *name;
  uint64_t bytes_sent;
  uint64_t bytes_recv;
  uint64_t packets_sent;
  uint64_t packets_recv;
  uint64_t errin;
  uint64_t errout;
  uint64_t dropin;
  uint64_t dropout;
} NetIOCounters;

typedef struct {
  uint32_t nitems;
  NetIOCounters *iocounters;
} NetIOCounterInfo;

typedef struct {
  char *username;
  char *tty;
  char *hostname;
  float tstamp;
} Users;

typedef struct {
  uint32_t nitems;
  Users *users;
} UsersInfo;

typedef struct {
  uint64_t total;
  uint64_t available;
  float percent;
  uint64_t used;
  uint64_t free;
  uint64_t active;
  uint64_t inactive;
  uint64_t buffers;
  uint64_t cached;
  uint64_t wired;
} VmemInfo;

typedef struct {
  uint64_t total;
  uint64_t used;
  uint64_t free;
  float percent;
  uint64_t sin;
  uint64_t sout;
} SwapMemInfo;

typedef struct
{
	double user;
	double system;
	double idle;
#ifdef _WIN32
	double interrupt;
	double dpc;
#else
	double nice;
	double iowait;
	double irq;
	double softirq;
	double steal;
	double guest;
	double guest_nice;
#endif
}CpuTimes;

typedef struct {
	uint32_t ctx_switches;
	uint32_t interrupts;
	uint32_t soft_interrupts;
	uint32_t syscalls;
#ifdef _WIN32
	uint32_t dpcs;
#endif
} cpustats;

typedef struct {
  pid_t pid;
  pid_t ppid;
  char *name;
  char *exe;
  char *cmdline;
  double create_time;
#ifdef _WIN32
  uint32_t num_handles; // num handles only available in windows
  enum con_status status;/* TODO : Implement others in this block in linux*/
  enum proc_priority nice;
  uint32_t num_ctx_switches; 
  uint32_t num_threads;
#else
  uint32_t uid; // this block is not available on windows
  uint32_t euid;
  uint32_t suid;
  uint32_t gid;
  uint32_t egid;
  uint32_t sgid;
  char *terminal;
#endif 
  char *username;
} Process;

bool disk_usage(const char[], DiskUsage *);

DiskPartitionInfo *disk_partitions(bool);
void free_disk_partition_info(DiskPartitionInfo *);

DiskIOCounterInfo *disk_io_counters(void);
void free_disk_iocounter_info(DiskIOCounterInfo *);

NetIOCounterInfo *net_io_counters(void); 
NetIOCounterInfo *net_io_counters_per_nic(void);
void free_net_iocounter_info(NetIOCounterInfo *);

UsersInfo *get_users(void);
void free_users_info(UsersInfo *);

uint32_t get_boot_time(void);

bool virtual_memory(VmemInfo *);
bool swap_memory(SwapMemInfo *);

CpuTimes *cpu_times(bool);

CpuTimes *cpu_times_percent(bool, CpuTimes *);

//double *cpu_util_percent(bool percpu, CpuTimes *prev_times);
cpustats *cpu_stats();
uint32_t cpu_count(bool);

bool pid_exists(pid_t);
uint32_t *pids(uint32_t *);

Process *get_process(pid_t);
void free_process(Process *);

enum proc_status status(pid_t pid); //faster function for finding status of a process (in Windows)

/* Required to avoid [-Wimplicit-function-declaration] for python bindings */
void gcov_flush(void);





