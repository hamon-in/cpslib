#ifndef __pslib_linux_h
#define __pslib_linux_h


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
  STATUS_WAITING
};

enum ioprio_class {
  IOPRIO_CLASS_NONE,
  IOPRIO_CLASS_RT,
  IOPRIO_CLASS_BE,
  IOPRIO_CLASS_IDLE
};

enum rlimit {
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
};

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
  ABOVE_NORMAL_PRIORITY_CLASS,
  BELOW_NORMAL_PRIORITY_CLASS,
  HIGH_PRIORITY_CLASS,
  IDLE_PRIORITY_CLASS,
  NORMAL_PRIORITY_CLASS,
  REALTIME_PRIORITY_CLASS
};


typedef struct {
  unsigned long total;
  unsigned long used;
  unsigned long free;
  float percent;
} DiskUsage;

typedef struct {
  char *device;
  char *mountpoint;
  char *fstype;
  char *opts;
} DiskPartition; /* TBD: Pluralise */

typedef struct {
  int nitems;
  DiskPartition *partitions;
} DiskPartitionInfo;

typedef struct {
  char *name;
  unsigned long readbytes;
  unsigned long writebytes;
  unsigned long reads;
  unsigned long writes;
  unsigned long readtime;
  unsigned long writetime;
} DiskIOCounters;

typedef struct {
  int nitems;
  DiskIOCounters *iocounters;
} DiskIOCounterInfo;

typedef struct {
  char * name;
  unsigned long bytes_sent;
  unsigned long bytes_recv;
  unsigned long packets_sent;
  unsigned long packets_recv;
  unsigned long errin;
  unsigned long errout;
  unsigned long dropin;
  unsigned long dropout;
} NetIOCounters;

typedef struct {
  int nitems;
  NetIOCounters *iocounters;
} NetIOCounterInfo;

typedef struct {
  char *username;
  char *tty;
  char *hostname;
  float tstamp;
} Users;

typedef struct {
  int nitems;
  Users *users;
} UsersInfo;

typedef struct {
  unsigned long total;
  unsigned long available;
  float percent;
  unsigned long used;
  unsigned long free;
  unsigned long active;
  unsigned long inactive;
  unsigned long buffers;
  unsigned long cached;
} VmemInfo;

typedef struct {
  unsigned long total;
  unsigned long used;
  unsigned long free;
  float percent;
  unsigned long sin;
  unsigned long sout;
} SwapMemInfo;

typedef struct {
  double user;
  double system;
  double idle;
  double nice;
  double iowait;
  double irq;
  double softirq;
  double steal;
  double guest;
  double guest_nice;
} CpuTimes;

typedef struct {
  unsigned int pid;
  unsigned int ppid;
  char *name;
  char *exe;
  char *cmdline;
  unsigned long create_time;
  unsigned int uid;
  unsigned int euid;
  unsigned int suid;
  unsigned int gid;
  unsigned int egid;
  unsigned int sgid;
  char *username;
  char *terminal;
} Process;

int disk_usage(char [], DiskUsage *);

DiskPartitionInfo *disk_partitions(int);
void free_disk_partition_info(DiskPartitionInfo *);

DiskIOCounterInfo *disk_io_counters();
void free_disk_iocounter_info(DiskIOCounterInfo *);

NetIOCounterInfo *net_io_counters();
void free_net_iocounter_info(NetIOCounterInfo *);

UsersInfo *get_users();
void free_users_info(UsersInfo *);

unsigned long int get_boot_time();

int virtual_memory(VmemInfo *);
int swap_memory(SwapMemInfo *);

CpuTimes *cpu_times(int);
int cpu_times_per_cpu(CpuTimes **);

double *cpu_times_percent(int, CpuTimes *, int);
int cpu_times_percent_per_cpu(CpuTimes **);

double cpu_percent();
int cpu_percent_per_cpu(double **);

int cpu_count(int);

Process *get_process(unsigned int);
void free_process(Process *);

#endif
// disk_io_counters_per_disk
// net_io_counters_per_nic
