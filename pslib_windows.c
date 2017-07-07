#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wtsapi32.h>
#include <stdio.h>
#include <stdlib.h>
#include <winternl.h>
#include <powrprof.h>
#include <Winsock2.h>
#include <ws2ipdef.h>
#include <Iphlpapi.h>
#include <Psapi.h>
#include <process.h>
#include <TlHelp32.h>
#include <tchar.h>
#include <shellapi.h>
#include <winioctl.h>
#if (_WIN32_WINNT >= 0x0600) // Windows Vista and above
#include <ws2tcpip.h>
#endif

#include "pslib.h"
#include "common.h"
#include "pslib_windows.h"

#pragma comment(lib, "WTSAPI32.lib")
#pragma comment(lib, "Advapi32.lib")
#pragma comment(lib, "Shell32.lib")
#pragma comment(lib, "PSAPI.lib")
// Link with Iphlpapi.lib
#pragma comment(lib, "IPHLPAPI.lib")

void __gcov_flush(void){}
//TO get Error message
PCHAR GetError() {
	CHAR errormessage[100] = { 0 };
	PCHAR err = NULL;
	if (GetLastError() != ERROR_NOSUCHPROCESS)
	{
		FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)errormessage, 255, NULL);
	}
	else
	{
		strncpy(errormessage, "NO SUCH PROCESS", 100);
	}
	err = (PCHAR)calloc(strlen(errormessage) + 1, sizeof(CHAR));
	check_mem(err);
	strcpy(err, errormessage);
	return err;
error:
	return NULL;
}
//used to convert null terminated WCHAR string to char string 
char *ConvertWcharToChar(WCHAR buffer[])
{
	char *tmp = NULL;
	int length = WideCharToMultiByte(CP_UTF8, 0, buffer, -1, NULL, 0, NULL, NULL);
	tmp = (char *)calloc(length + 1, sizeof(char));
	check_mem(tmp);
	if (WideCharToMultiByte(CP_UTF8, 0, buffer, -1, tmp, length, NULL, NULL) == 0)
	{
		DWORD t;
		GetError(&t);
		goto error;
	}
	return tmp;
error:
	if (tmp)
		free(tmp);
	return NULL;
}

DiskIOCounterInfo *disk_io_counters(void) {
	DISK_PERFORMANCE_WIN_2008 diskPerformance;
	DWORD dwSize;
	HANDLE hDevice = NULL;
	char szDevice[MAX_PATH];
	char *szDeviceDisplay=NULL; 
	int devNum;
	DiskIOCounterInfo *ret = NULL;
	ret = (DiskIOCounterInfo *)calloc(1, sizeof(DiskIOCounterInfo));
	check_mem(ret);
	ret->iocounters = (DiskIOCounters *)calloc(1, sizeof(DiskIOCounters));
	check_mem(ret->iocounters);
	ret->nitems = 0;
	for (devNum = 0; devNum <= 32; ++devNum) {
		sprintf_s(szDevice, MAX_PATH, "\\\\.\\PhysicalDrive%d", devNum);
		hDevice = CreateFile(szDevice, 0, FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL, OPEN_EXISTING, 0, NULL);

		if (hDevice == INVALID_HANDLE_VALUE)
		{
			continue;
		}
		if (DeviceIoControl(hDevice, IOCTL_DISK_PERFORMANCE, NULL, 0,
			&diskPerformance, sizeof(diskPerformance),
			&dwSize, NULL))
		{
			ret->nitems++;
			ret->iocounters = (DiskIOCounters *)realloc(ret->iocounters , ret->nitems * sizeof(DiskIOCounters));
			check_mem(ret->iocounters);
			szDeviceDisplay = (char *)calloc(MAX_PATH, sizeof(char));
			sprintf_s(szDeviceDisplay, MAX_PATH, "PhysicalDrive%d", devNum);
			ret->iocounters[devNum].name = szDeviceDisplay;
			ret->iocounters[devNum].reads = diskPerformance.ReadCount;
			ret->iocounters[devNum].writes = diskPerformance.WriteCount;
			ret->iocounters[devNum].readbytes = diskPerformance.BytesRead.QuadPart;
			ret->iocounters[devNum].writebytes = diskPerformance.BytesWritten.QuadPart;
			ret->iocounters[devNum].readtime = (unsigned long long)(diskPerformance.ReadTime.QuadPart * 10) / 1000;
			ret->iocounters[devNum].writetime = (unsigned long long)(diskPerformance.WriteTime.QuadPart * 10) / 1000;
		}
		CloseHandle(hDevice);
	}
	return ret;
error:
	if (ret)
		free_disk_iocounter_info(ret);
	if (hDevice != NULL)
		CloseHandle(hDevice);
	return NULL;
}
void free_disk_iocounter_info(DiskIOCounterInfo *tmp)
{
	free(tmp->iocounters);
	free(tmp);
}

static char *get_drive_type(int type) {
	switch (type) {
	case DRIVE_FIXED:
		return "fixed";
	case DRIVE_CDROM:
		return "cdrom";
	case DRIVE_REMOVABLE:
		return "removable";
	case DRIVE_UNKNOWN:
		return "unknown";
	case DRIVE_NO_ROOT_DIR:
		return "unmounted";
	case DRIVE_REMOTE:
		return "remote";
	case DRIVE_RAMDISK:
		return "ramdisk";
	default:
		return "?";
	}
}

DiskPartitionInfo *disk_partitions(bool all) {
	DWORD num_bytes;
	char *drive_letter = NULL;
	int type;
	int _ret;
	unsigned int old_mode = 0;
	char *tmpstring;
	LPTSTR fs_type[MAX_PATH + 1] = { 0 };
	DWORD pflags = 0;
	char opts[20];
	DiskPartitionInfo *ret = NULL; 
	ret = (DiskPartitionInfo *)calloc(1,sizeof(DiskPartitionInfo));
	check_mem(ret);
	ret->nitems = 0;
	drive_letter = (char *)calloc(1, 255 * sizeof(char));
	check_mem(drive_letter);
	// avoid to visualize a message box in case something goes wrong
	// see https://github.com/giampaolo/psutil/issues/264
	old_mode = SetErrorMode(SEM_FAILCRITICALERRORS);
	num_bytes = GetLogicalDriveStrings(254, drive_letter);
	check(num_bytes, "Error in getting valid drives");
	while (*drive_letter != '\0') {
		opts[0] = 0;
		fs_type[0] = 0;
		ret->nitems++;
		ret->partitions = (DiskPartition *)realloc(ret->partitions, ret->nitems * sizeof(DiskPartition));
		type = GetDriveType(drive_letter);

			// by default we only show hard drives and cd-roms
		if (all==0) {
			if ((type == DRIVE_UNKNOWN) ||
				(type == DRIVE_NO_ROOT_DIR) ||
				(type == DRIVE_REMOTE) ||
				(type == DRIVE_RAMDISK)) {
				goto next;
			}
			// floppy disk: skip it by default as it introduces a
			// considerable slowdown.
			if ((type == DRIVE_REMOVABLE) &&
				(strcmp(drive_letter, "A:\\") == 0)) {
				goto next;
			}
		}
		_ret = GetVolumeInformation(
			(LPCTSTR)drive_letter, NULL, _ARRAYSIZE(drive_letter),
			NULL, NULL, &pflags, (LPTSTR)fs_type, _ARRAYSIZE(fs_type));
		if (_ret == 0) {
			// We might get here in case of a floppy hard drive, in
			// which case the error is (21, "device not ready").
			// Let's pretend it didn't happen as we already have
			// the drive name and type ('removable').
			strcat_s(opts, _countof(opts), "");
			SetLastError(0);
		}
		else {
			if (pflags & FILE_READ_ONLY_VOLUME)
				strcat_s(opts, _countof(opts), "ro");
			else
				strcat_s(opts, _countof(opts), "rw");
			if (pflags & FILE_VOLUME_IS_COMPRESSED)
				strcat_s(opts, _countof(opts), ",compressed");
		}

		if (strlen(opts) > 0)
			strcat_s(opts, _countof(opts), ",");
		strcat_s(opts, _countof(opts), get_drive_type(type));
		tmpstring = (char *)calloc(1, 20 * sizeof(char));
		check_mem(tmpstring);
		strcpy(tmpstring, opts);
		ret->partitions[ret->nitems - 1].opts = tmpstring;
		tmpstring = (char *)calloc(1, (MAX_PATH + 1) * sizeof(char));
		check_mem(tmpstring);
		strcpy(tmpstring, (char *)fs_type);
		ret->partitions[ret->nitems - 1].device = drive_letter;
		ret->partitions[ret->nitems - 1].mountpoint = drive_letter;
		ret->partitions[ret->nitems - 1].fstype = tmpstring;
		goto next;

	next:
		drive_letter = strchr(drive_letter, 0) + 1;
	}
	SetErrorMode(old_mode);
	return ret;

error:
	SetErrorMode(old_mode);
	if (drive_letter)
		free(drive_letter);
	if (ret)
		free_disk_partition_info(ret);
	return NULL;
}
void free_disk_partition_info(DiskPartitionInfo *p)
{
	uint32_t i;
	free(p->partitions[0].device);
	for ( i = 0; i < p->nitems; i++)
	{
		free(p->partitions[i].fstype);
		free(p->partitions[i].opts);
	}
	free(p->partitions);
	free(p);
}

bool disk_usage(const char path[], DiskUsage *disk) {
	BOOL retval;
	ULARGE_INTEGER _, total, free;
		retval = GetDiskFreeSpaceExA((LPCSTR)path, &_, &total, &free);
		if (retval)
		{
			disk->total = total.QuadPart;
			disk->free = free.QuadPart;
			disk->used = disk->total - disk->free;
			disk->percent = percentage(disk->used, disk->total);
			return true;
		}
		else
		{
			return false;
		}
}
void free_users_info(UsersInfo *u)
{
	uint32_t i;
	for ( i = 0; i < u->nitems; i++)
	{
		if(u->users[i].hostname)
			free(u->users[i].hostname);
		if (u->users[i].tty)
			free(u->users[i].tty);
		if (u->users[i].username)
			free(u->users[i].username);
	}
	free(u->users);
	free(u);
}
UsersInfo *get_users() {
	HANDLE hServer = WTS_CURRENT_SERVER_HANDLE;
	WCHAR *buffer_user = NULL;
	LPTSTR buffer_addr = NULL;
	PWTS_SESSION_INFO sessions = NULL;
	DWORD count;
	DWORD i;
	DWORD sessionId;
	DWORD bytes;
	PWTS_CLIENT_ADDRESS address;
	char address_str[50];
	long long unix_time;
	char *tmp = NULL;
	PWINSTATIONQUERYINFORMATIONW WinStationQueryInformationW;
	WINSTATION_INFO station_info;
	HINSTANCE hInstWinSta = NULL;
	ULONG returnLen;

	UsersInfo *users_info_ptr = NULL;
	hInstWinSta = LoadLibraryA("winsta.dll");
	WinStationQueryInformationW = (PWINSTATIONQUERYINFORMATIONW) \
		GetProcAddress(hInstWinSta, "WinStationQueryInformationW");
	users_info_ptr = (UsersInfo *)calloc(1, sizeof(UsersInfo));
	check_mem(users_info_ptr);
	check(WTSEnumerateSessions(hServer, 0, 1, &sessions, &count),
			"error in retrieving list of sessions on a Remote Desktop Session Host server");
	users_info_ptr->users = (Users *)calloc(count, sizeof(Users));
	check_mem(users_info_ptr->users);
	users_info_ptr->nitems = 0;
	for (i = 0; i < count; i++) {
		sessionId = sessions[i].SessionId;
		if (buffer_user != NULL)
			WTSFreeMemory(buffer_user);
		if (buffer_addr != NULL)
			WTSFreeMemory(buffer_addr);

		buffer_user = NULL;
		buffer_addr = NULL;
 
		// username
		bytes = 0;
		check(WTSQuerySessionInformationW(hServer, sessionId, WTSUserName,
			&buffer_user, &bytes), "error in retriving Username");
		if (bytes <= 2)
			continue;
		// address
		bytes = 0;
		check( WTSQuerySessionInformation(hServer, sessionId, WTSClientAddress,
			&buffer_addr, &bytes),"error in retriving ClientAddress") 

		address = (PWTS_CLIENT_ADDRESS)buffer_addr;
		if (address->AddressFamily == 0) {  // AF_INET
			sprintf_s(address_str,
				_countof(address_str),
				"%u.%u.%u.%u",
				address->Address[0],
				address->Address[1],
				address->Address[2],
				address->Address[3]);
			tmp = (char *)calloc(50, sizeof(char));
			check_mem(tmp);
			strcpy(tmp,address_str);
			users_info_ptr->users[users_info_ptr->nitems].hostname = tmp;
		}
		else {
			users_info_ptr->users[users_info_ptr->nitems].hostname = NULL;
		}

		// login time
		check(WinStationQueryInformationW(hServer,
			sessionId,
			WinStationInformation,
			&station_info,
			sizeof(station_info),
			&returnLen), "error in retriving login time");
		unix_time = ((LONGLONG)station_info.ConnectTime.dwHighDateTime) << 32;
		unix_time += \
			station_info.ConnectTime.dwLowDateTime - 116444736000000000LL;
		unix_time /= 10000000;
		tmp = ConvertWcharToChar(buffer_user);
		check(tmp, "Error in converting wchar");
		users_info_ptr->users[users_info_ptr->nitems].username = tmp;
		users_info_ptr->users[users_info_ptr->nitems].tstamp = (double)unix_time;
		users_info_ptr->users[users_info_ptr->nitems].tty = (char *)calloc(1, sizeof(char));
		check_mem(users_info_ptr->users[users_info_ptr->nitems].tty);
		users_info_ptr->users[users_info_ptr->nitems].tty[0] = '\0';
		users_info_ptr->nitems++;
	}

	WTSFreeMemory(sessions);
	WTSFreeMemory(buffer_user);
	WTSFreeMemory(buffer_addr);
	FreeLibrary(hInstWinSta);
	return users_info_ptr;
error:
	if (hInstWinSta != NULL)
		FreeLibrary(hInstWinSta);
	if (sessions != NULL)
		WTSFreeMemory(sessions);
	if (buffer_user != NULL)
		WTSFreeMemory(buffer_user);
	if (buffer_addr != NULL)
		WTSFreeMemory(buffer_addr);
	if (users_info_ptr)
		free_users_info(users_info_ptr);
	if (tmp)
		free(tmp);
	return NULL;
}

double get_valid_value(double a)
{
	if (a <= 0.0)
		return 0.0;
	else if (a > 100.0)
		return 100.0;
	else
		return a;
}
CpuTimes *calculate(CpuTimes *t1, CpuTimes * t2)
{
	double all_delta;
	int i;
	CpuTimes *ret;
	ret = (CpuTimes *)calloc(1,sizeof(CpuTimes));
	check_mem(ret);
	all_delta = (t2->user + t2->system + t2->interrupt + t2->idle + t2->dpc) - \
		(t1->user + t1->system + t1->interrupt + t1->idle + t1->dpc);
	if (all_delta==0)
		all_delta = 100;
	ret->user = get_valid_value((t2->user - t1->user)*100.0 / all_delta);
	ret->system = get_valid_value((t2->system - t1->system)*100.0 / all_delta);
	ret->interrupt = get_valid_value((t2->interrupt - t1->interrupt)*100.0 / all_delta);
	ret->idle = get_valid_value((t2->idle - t1->idle)*100.0 / all_delta);
	ret->dpc = get_valid_value((t2->dpc - t1->dpc)*100.0 / all_delta);
	return ret;
error:
	if (ret)
		free(ret);
	return NULL;
}

bool swap_memory(SwapMemInfo *ret)
{
	MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);

    if (! GlobalMemoryStatusEx(&memInfo))
        return false;
            
    ret->total = memInfo.ullTotalPageFile;
    ret->free = memInfo.ullAvailPageFile;
    ret->used = ret->total - ret->free;
	ret->percent = (ret->used)*100.0/ret->total;
	ret->sin=0;
	ret->sout=0;
	return true;	
}

bool virtual_memory(VmemInfo *ret)
{
	MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);

    if (! GlobalMemoryStatusEx(&memInfo))
        return false;
            
    ret->total = memInfo.ullTotalPhys;
    ret->available = memInfo.ullAvailPhys;
    ret->used = ret->total - ret->available;
	ret->percent = (ret->used)*100.0/ret->total;
	ret->free = ret->available;
	return true;
}



unsigned int cpu_count_logical() {
    SYSTEM_INFO system_info;
    system_info.dwNumberOfProcessors = 0;

    GetSystemInfo(&system_info);
    if (system_info.dwNumberOfProcessors == 0)
        return 0;
    else
        return system_info.dwNumberOfProcessors;
}

unsigned int cpu_count_phys() {
    LPFN_GLPI glpi;
    DWORD rc;
    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION buffer = NULL;
    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION ptr = NULL;
    DWORD length = 0;
    DWORD offset = 0;
    unsigned int ncpus = 0;

    glpi = (LPFN_GLPI)GetProcAddress(GetModuleHandle(TEXT("kernel32")),
                                     "GetLogicalProcessorInformation");
    check(glpi,"");

    while (1) {
        rc = glpi(buffer, &length);
        if (rc == FALSE) {
            if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
                if (buffer)
                    free(buffer);
				buffer = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION)calloc(length, sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION));
                check_mem(buffer);
            }
            else {
                goto error;
            }
        }
        else {
            break;
        }
    }

    ptr = buffer;
    while (offset + sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION) <= length) {
        if (ptr->Relationship == RelationProcessorCore)
            ncpus += 1;
        offset += sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
        ptr++;
    }

    free(buffer);
    if (ncpus == 0)
        goto error;
    else
        return ncpus;

error:
    
    if (buffer != NULL)
        free(buffer);
    return 0;
}

unsigned int cpu_count(bool logical)
{
	if(logical)
	return cpu_count_logical();
	else
	return cpu_count_phys();
}
uint32_t get_boot_time() {
#if (_WIN32_WINNT >= 0x0600)  // Windows Vista
    ULONGLONG uptime;
#else
    double uptime;

#endif
    time_t pt;
    FILETIME fileTime;
    long long ll;
    HINSTANCE hKernel32;
    psutil_GetTickCount64 = NULL;

    GetSystemTimeAsFileTime(&fileTime);

    ll = (((LONGLONG)(fileTime.dwHighDateTime)) << 32) 
        + fileTime.dwLowDateTime;
    pt = (time_t)((ll - 116444736000000000ull) / 10000000ull);

    hKernel32 = GetModuleHandleW(L"KERNEL32");
    psutil_GetTickCount64 = (void*)GetProcAddress(hKernel32, "GetTickCount64");
    if (psutil_GetTickCount64 != NULL) {
        // Windows >= Vista
        uptime = psutil_GetTickCount64() / (ULONGLONG)1000.00f;
    }
    else {
        // Windows XP.
        uptime = GetTickCount() / 1000.00f;
    }

    return pt - uptime;
}

CpuTimes* per_cpu_times() {
    // NtQuerySystemInformation stuff
    typedef DWORD (_stdcall * NTQSI_PROC) (int, PVOID, ULONG, PULONG);
    NTQSI_PROC NtQuerySystemInformation;
    HINSTANCE hNtDll;
	CpuTimes *ret = NULL;
	CpuTimes *c = NULL;
    float idle, kernel, systemt, user, interrupt, dpc;
    NTSTATUS status;
    _SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION *sppi = NULL;
    SYSTEM_INFO si;
    UINT i;
    // obtain NtQuerySystemInformation
    hNtDll = LoadLibrary(TEXT("ntdll.dll"));
    check(hNtDll,"Error in loading library 'ntdll'");
    NtQuerySystemInformation = (NTQSI_PROC)GetProcAddress(
        hNtDll, "NtQuerySystemInformation");
   
    check(NtQuerySystemInformation, "Unable to fetch NtQuerySystemInformation");

    // retrives number of processors
    GetSystemInfo(&si);

    // allocates an array of _SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION
    // structures, one per processor
    sppi = (_SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION *) \
           malloc(si.dwNumberOfProcessors * \
                  sizeof(_SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION));
    check(sppi,"Unable to allocate array of SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION");

    // gets cpu time informations
    status = NtQuerySystemInformation(
        SystemProcessorPerformanceInformation,
        sppi,
        si.dwNumberOfProcessors * sizeof
            (_SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION),
        NULL);
    check(status==0,"Unable to get CPU time");
    
	
	ret = (CpuTimes*)calloc(si.dwNumberOfProcessors,sizeof(CpuTimes));
	c = ret;
	if(!ret)
		goto error;
    // computes system global times summing each
    // processor value
    idle = user = kernel = interrupt = dpc = 0;
    for (i = 0; i < si.dwNumberOfProcessors; i++) {
        user = (float)((HI_T * sppi[i].UserTime.HighPart) +
                       (LO_T * sppi[i].UserTime.LowPart));
        idle = (float)((HI_T * sppi[i].IdleTime.HighPart) +
                       (LO_T * sppi[i].IdleTime.LowPart));
        kernel = (float)((HI_T * sppi[i].KernelTime.HighPart) +
                         (LO_T * sppi[i].KernelTime.LowPart));
        interrupt = (float)((HI_T * sppi[i].InterruptTime.HighPart) +
                            (LO_T * sppi[i].InterruptTime.LowPart));
        dpc = (float)((HI_T * sppi[i].DpcTime.HighPart) +
                      (LO_T * sppi[i].DpcTime.LowPart));

        // kernel time includes idle time on windows
        // we return only busy kernel time subtracting
        // idle time from kernel time
        systemt = kernel - idle;
        
        c->user=user;
        c->system=systemt;
        c->interrupt=interrupt;
        c->idle=idle;
        c->dpc=dpc;
        ++c;
    }

    free(sppi);
    FreeLibrary(hNtDll);
    return ret;

error:
    if (sppi)
        free(sppi);
    if (hNtDll)
        FreeLibrary(hNtDll);
    return NULL;
}
CpuTimes *sum_per_cpu_times(CpuTimes *per_cpu)
{
	unsigned int i;
	CpuTimes *ret = NULL, *c = NULL;
	double user,system,idle,dpc,interrupt;
	ret = (CpuTimes*)calloc(1,sizeof(CpuTimes));
	check_mem(ret);
	idle = user = system = interrupt = dpc = 0;
	c = per_cpu;
	for(i = 0; i < cpu_count(true); ++i)
	{
		user += c->user;
		system += c->system;
		idle += c->idle;
		interrupt += c->interrupt;
		dpc += c->dpc;
		++c;
	}
	ret->user=user;
	ret->system=system;
	ret->idle=idle;
	ret->interrupt=interrupt;
	ret->dpc=dpc;
	return ret;
error:
	return NULL;
}
CpuTimes *cpu_times_summed()
 {
    float idle, kernel, user, system;
    FILETIME idle_time, kernel_time, user_time;
	CpuTimes *data = NULL, *percpu = NULL, *percpu_summed = NULL;
	check(GetSystemTimes(&idle_time, &kernel_time, &user_time), "failed to get System times");
    idle = (float)((HI_T * idle_time.dwHighDateTime) + \
                   (LO_T * idle_time.dwLowDateTime));
    user = (float)((HI_T * user_time.dwHighDateTime) + \
                   (LO_T * user_time.dwLowDateTime));
    kernel = (float)((HI_T * kernel_time.dwHighDateTime) + \
                     (LO_T * kernel_time.dwLowDateTime));

    // Kernel time includes idle time.
    // We return only busy kernel time subtracting idle time from
    // kernel time.
    system = (kernel - idle);
    data = (CpuTimes*)calloc(1,sizeof(CpuTimes));
    check_mem(data);
    data->idle=idle;
    data->system=system;
    data->user=user;
    
	percpu = per_cpu_times();
    check_mem(percpu);
	percpu_summed = sum_per_cpu_times(percpu);
    check_mem(percpu_summed);
    free(percpu);
	data->interrupt = percpu_summed->interrupt;
    data->dpc= percpu_summed->dpc;
    free(percpu_summed);
    return data;
error:
	if (data)
		free(data);
	if (percpu)
		free(percpu);
	if (percpu_summed)
		free(percpu_summed);
	return NULL;
}
CpuTimes *cpu_times(bool percpu)
{
	CpuTimes *ret;
	if(!percpu)
	{
		return  cpu_times_summed();
	}
	else
		return  per_cpu_times();
}

cpustats *cpu_stats() {
    // NtQuerySystemInformation stuff
    typedef DWORD (_stdcall * NTQSI_PROC) (int, PVOID, ULONG, PULONG);
    NTQSI_PROC NtQuerySystemInformation;
    HINSTANCE hNtDll;
	cpustats *ret = NULL;
    NTSTATUS status;
    _SYSTEM_PERFORMANCE_INFORMATION *spi = NULL;
    _SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION *sppi = NULL;
    _SYSTEM_INTERRUPT_INFORMATION *InterruptInformation = NULL;
    SYSTEM_INFO si;
    UINT i;
    ULONG64 dpcs = 0;
    ULONG interrupts = 0;

    // obtain NtQuerySystemInformation
    hNtDll = LoadLibrary(TEXT("ntdll.dll"));
    check(hNtDll,"Unable to fetch library 'ntdll'");
    
    NtQuerySystemInformation = (NTQSI_PROC)GetProcAddress(
        hNtDll, "NtQuerySystemInformation");
  
    check(NtQuerySystemInformation, "Unable to fetch NtQuerySystemInformation");

    // retrives number of processors
    GetSystemInfo(&si);

    // get syscalls / ctx switches
    spi = (_SYSTEM_PERFORMANCE_INFORMATION *) \
           malloc(si.dwNumberOfProcessors * \
                  sizeof(_SYSTEM_PERFORMANCE_INFORMATION));
    check_mem(spi);
    status = NtQuerySystemInformation(
        SystemPerformanceInformation,
        spi,
        si.dwNumberOfProcessors * sizeof(_SYSTEM_PERFORMANCE_INFORMATION),
        NULL);
  
    check(status==0,"");

    // get DPCs
    InterruptInformation = (_SYSTEM_INTERRUPT_INFORMATION *)calloc(si.dwNumberOfProcessors, sizeof(_SYSTEM_INTERRUPT_INFORMATION));
   
    check_mem(InterruptInformation);
    status = NtQuerySystemInformation(
        SystemInterruptInformation,
        InterruptInformation,
        si.dwNumberOfProcessors * sizeof(SYSTEM_INTERRUPT_INFORMATION),
        NULL);
    check(status==0,"");
    
    for (i = 0; i < si.dwNumberOfProcessors; i++) {
        dpcs += InterruptInformation[i].DpcCount;
    }

    // get interrupts
    sppi = (_SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION *) \
        malloc(si.dwNumberOfProcessors * \
               sizeof(_SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION));
    check_mem(sppi);

    status = NtQuerySystemInformation(
        SystemProcessorPerformanceInformation,
        sppi,
        si.dwNumberOfProcessors * sizeof
            (_SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION),
        NULL);
    
    check(status==0,"");

    for (i = 0; i < si.dwNumberOfProcessors; i++) {
        interrupts += sppi[i].InterruptCount;
    }

    // done
    free(spi);
    free(InterruptInformation);
    free(sppi);
    FreeLibrary(hNtDll);
    
    
    ret =  (cpustats*)calloc(1,sizeof(cpustats));
    check_mem(ret);
    ret->ctx_switches = spi->ContextSwitches;
    ret->interrupts = interrupts;
    ret->dpcs = (unsigned long)dpcs;
    ret->soft_interrupts = 0;
    ret->syscalls = spi->SystemCalls;
    
    return ret;


error:
    if (spi)
        free(spi);
    if (InterruptInformation)
        free(InterruptInformation);
    if (sppi)
        free(sppi);
    if (hNtDll)
        FreeLibrary(hNtDll);
    return NULL;
}
/*Same as cpu_percent() but provides utilization percentages
		for each specific CPU time as is returned by cpu_times().*/			
CpuTimes *cpu_times_percent(bool percpu , CpuTimes * Last_Cpu_times)
{
	uint32_t no_cpu, i;
	CpuTimes *t2 = NULL, *ret = NULL, *tmp = NULL;
	t2 = (CpuTimes *)calloc(1, sizeof(CpuTimes));
	check_mem(t2);
	if (!percpu) {			
			t2 = cpu_times(false);
			ret = calculate(t2, Last_Cpu_times);
		}
//		per - cpu usage
	else {
		ret = (CpuTimes *)calloc(1, sizeof(CpuTimes));
		check_mem(ret);
		t2 = cpu_times(true);

		no_cpu = cpu_count(true);
		for (i = 0; i < no_cpu; i++)
		{
			ret = (CpuTimes *)realloc(ret, (i + 1) * sizeof(CpuTimes));
			check_mem(ret);
			tmp = calculate(&t2[i], &Last_Cpu_times[i]);
			check(tmp, "Error in calculting difference");
			ret[i].user = tmp->user;
			ret[i].system = tmp->system;
			ret[i].interrupt = tmp->interrupt;
			ret[i].idle = tmp->idle;
			ret[i].dpc = tmp->dpc;
			free(tmp);
		}	
	}	
	return ret;
error:
	if (tmp)
		free(tmp);
	if (t2)
		free(t2);
	if (ret)
		free(ret);
	return NULL;
}

uint32_t *pids(uint32_t *numberOfReturnedPIDs) {


    DWORD *procArray = NULL;
    DWORD procArrayByteSz;
    int procArraySz = 0;

    // Stores the byte size of the returned array from enumprocesses
    DWORD enumReturnSz = 0;

    do {
        procArraySz += 1024;
        free(procArray);
        procArrayByteSz = procArraySz * sizeof(DWORD);
        procArray = (DWORD *)calloc(1, procArrayByteSz);
        
	check_mem(procArray);
       
	check(K32EnumProcesses(procArray, procArrayByteSz, &enumReturnSz),"Unable to enumerate processes");
    } while (enumReturnSz == procArraySz * sizeof(DWORD));

    // The number of elements is the returned size / size of each element
    *numberOfReturnedPIDs = enumReturnSz / sizeof(DWORD);

    check(procArray,"");
	    
    return procArray;
 error:

    if (procArray != NULL)
      free(procArray);
    return NULL;
}

int pid_is_running(DWORD pid) {
    HANDLE hProcess;
    DWORD exitCode;

    // Special case for PID 0 System Idle Process
    if (pid == 0)
        return 1;
    if (pid < 0)
        return 0;

    hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,FALSE, pid);
    if (NULL == hProcess) {
        // invalid parameter is no such process
        if (GetLastError() == ERROR_INVALID_PARAMETER) {
            CloseHandle(hProcess);
            return 0;
        }

        // access denied obviously means there's a process to deny access to...
        if (GetLastError() == ERROR_ACCESS_DENIED) {
            CloseHandle(hProcess);
            return 1;
        }
        CloseHandle(hProcess);
        return -1;
    }

    if (GetExitCodeProcess(hProcess, &exitCode)) {
        CloseHandle(hProcess);
        return (exitCode == STILL_ACTIVE);
    }

    // access denied means there's a process there so we'll assume
    // it's running
    if (GetLastError() == ERROR_ACCESS_DENIED) {
        CloseHandle(hProcess);
        return 1;
    }
    CloseHandle(hProcess);
    return -1;
}

bool pid_exists(pid_t pid) 
{
    int status;
    status = pid_is_running(pid);
    check(status!=-1,"Exception raised in pid_is_running");
    return status;
 error:
    return false;
}


PIP_ADAPTER_ADDRESSES get_nic_addresses() {
    // allocate a 15 KB buffer to start with
    ULONG outBufLen = 15000;
    DWORD dwRetVal = 0;
    ULONG attempts = 0;
    PIP_ADAPTER_ADDRESSES pAddresses = NULL;

    do {
        pAddresses = (IP_ADAPTER_ADDRESSES *) malloc(outBufLen);
		check_mem(pAddresses);

        dwRetVal = GetAdaptersAddresses(AF_UNSPEC, 0, NULL, pAddresses, &outBufLen);
        if (dwRetVal == ERROR_BUFFER_OVERFLOW) {
            free(pAddresses);
            pAddresses = NULL;
        }
        else {
            break;
        }

        attempts++;
    } while ((dwRetVal == ERROR_BUFFER_OVERFLOW) && (attempts < 3));

	check(dwRetVal == NO_ERROR, "GetAdaptersAddresses() syscall failed.");
    return pAddresses;
error:
	if (pAddresses)
		free(pAddresses);
	return NULL;
}


NetIOCounterInfo *net_io_counters_per_nic() 
{
	int len;
    DWORD dwRetVal = 0;
	NetIOCounterInfo *ret = NULL;
	NetIOCounters *counters = NULL, *nc = NULL;
#if (_WIN32_WINNT >= 0x0600) // Windows Vista and above
    MIB_IF_ROW2 *pIfRow = NULL;
#else // Windows XP
    MIB_IFROW *pIfRow = NULL;
#endif

    PIP_ADAPTER_ADDRESSES pAddresses = NULL;
    PIP_ADAPTER_ADDRESSES pCurrAddresses = NULL;
    pAddresses = get_nic_addresses();
    if (pAddresses == NULL)
        goto error;
    pCurrAddresses = pAddresses;
    ret = (NetIOCounterInfo*) calloc(1,sizeof(NetIOCounterInfo));
	check_mem(ret);
    counters = (NetIOCounters*) calloc(15,sizeof(NetIOCounters));
	check_mem(counters);
    nc = counters;
    ret->nitems = 0;
    ret->iocounters = counters;

    while (pCurrAddresses) {

#if (_WIN32_WINNT >= 0x0600) // Windows Vista and above
      pIfRow = (MIB_IF_ROW2 *) malloc(sizeof(MIB_IF_ROW2));
#else // Windows XP
        pIfRow = (MIB_IFROW *) malloc(sizeof(MIB_IFROW));
#endif
	
       check_mem(pIfRow)

#if (_WIN32_WINNT >= 0x0600) // Windows Vista and above
        SecureZeroMemory((PVOID)pIfRow, sizeof(MIB_IF_ROW2));
        pIfRow->InterfaceIndex = pCurrAddresses->IfIndex;
        dwRetVal = GetIfEntry2(pIfRow);
#else // Windows XP
        pIfRow->dwIndex = pCurrAddresses->IfIndex;
        dwRetVal = GetIfEntry(pIfRow);
#endif
	
		check(dwRetVal == NO_ERROR, "GetIfEntry() or GetIfEntry2() syscalls failed.");
	
#if (_WIN32_WINNT >= 0x0600) // Windows Vista and above
	
	nc->bytes_sent = pIfRow->OutOctets;
	nc->bytes_recv = pIfRow->InOctets;
	nc->packets_sent = pIfRow->OutUcastPkts;
	nc->packets_recv = pIfRow->InUcastPkts;
	nc->errin = pIfRow->InErrors;
	nc->errout = pIfRow->OutErrors;
	nc->dropin = pIfRow->InDiscards;
	nc->dropout = pIfRow->OutDiscards;

	
#else // Windows XP
	nc->bytes_sent = pIfRow->dwOutOctets;
	nc->bytes_recv = pIfRow->dwInOctets;
	nc->packets_sent = pIfRow->dwOutUcastPkts;
	nc->packets_recv = pIfRow->dwInUcastPkts;
	nc->errin = pIfRow->dwInErrors;
	nc->errout = pIfRow->dwOutErrors;
	nc->dropin = pIfRow->dwInDiscards;
	nc->dropout = pIfRow->dwOutDiscards;

#endif

	nc->name = (char *)calloc(wcslen(pCurrAddresses->FriendlyName) + 1, sizeof(char));
	len = wcslen(pCurrAddresses->FriendlyName);
	pCurrAddresses->FriendlyName[len] = '\0';
	nc->name = ConvertWcharToChar(pCurrAddresses->FriendlyName);
	ret->nitems++;
	nc++;
    free(pIfRow);
    pCurrAddresses = pCurrAddresses->Next;
    }

    free(pAddresses);
    return ret;
    
error:
    if (pAddresses != NULL)
      free(pAddresses);
    if (pIfRow != NULL)
        free(pIfRow);
    if(ret!=NULL)
      free(ret);
    return NULL;
}
void free_net_iocounter_info(NetIOCounterInfo *ret)
{
	int i;
	NetIOCounters *c=ret->iocounters;
	for(i=0;i<ret->nitems;++i)
	{
		free(c->name);
		++c;
	}
	free(ret->iocounters);
	free(ret);
}

NetIOCounterInfo *net_io_counters_summed(NetIOCounterInfo *info)
{
	NetIOCounterInfo *sum = NULL;
	NetIOCounters *r = NULL,*c =NULL;
	int i;
	sum =(NetIOCounterInfo*) calloc(1,sizeof(NetIOCounterInfo));
	check_mem(sum);
	r = (NetIOCounters*) calloc(1,sizeof(NetIOCounters));
	check_mem(r);
	sum->iocounters = r;
	sum->nitems = 1;

	c =info->iocounters;
	for(i=0;i<info->nitems;++i)
	{
		r->bytes_recv+=c->bytes_recv;
		r->bytes_sent+=c->bytes_sent;
		r->dropin+=c->dropin;
		r->dropout+=c->dropout;
		r->errout+=c->errout;
		r->errin+=c->errin;
		r->packets_recv+=c->packets_recv;
		r->packets_sent+=c->packets_sent;
		++c;
	}
	free_net_iocounter_info(info);
	return sum;
error:
	if(sum)
		free(sum);
	if(r)
		free(r);
	return info;
}
NetIOCounterInfo *net_io_counters()
{
	return net_io_counters_per_nic();
}

pid_parent_map *ppid_map() {
	HANDLE handle = NULL;
	PROCESSENTRY32 pe = { 0 };
	pid_parent_map *ret = NULL;
	ret = (pid_parent_map *)calloc(1, sizeof(pid_parent_map));
	check_mem(ret);
	ret->pid = (pid_t *)calloc(1, sizeof(pid_t));
	check_mem(ret->pid);
	ret->ppid = (pid_t *)calloc(1, sizeof(pid_t));
	check_mem(ret->ppid);
	pe.dwSize = sizeof(PROCESSENTRY32);
	handle = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	check(handle, "Error in creating handle");
	ret->nitems = 0;
	if (Process32First(handle, &pe)) {
		do {
			ret->pid = (pid_t *)realloc(ret->pid, (ret->nitems + 1) * sizeof(pid_t));
			check_mem(ret->pid);
			ret->ppid = (pid_t *)realloc(ret->ppid, (ret->nitems + 1) * sizeof(pid_t));
			check_mem(ret->ppid);
			ret->pid[ret->nitems] = pe.th32ProcessID;
			ret->ppid[ret->nitems] = pe.th32ParentProcessID;
			ret->nitems++;
		} while (Process32Next(handle, &pe));
	}
	CloseHandle(handle);
	return ret;
error:
	if (ret)
		free(ret);
	if (handle)
		CloseHandle(handle);
	return NULL;
}
pid_t ppid(pid_t pid)
{
	pid_parent_map *map = NULL;
	uint32_t i;
	pid_t ret = -1;
	map = ppid_map();
	check(map,"Error in retriving ppid_map");
	for ( i = 0; i < map->nitems; i++)
	{
		if (map->pid[i] == pid)
		{
			ret = map->ppid[i];
			break;
		}
	}
	check(i != map->nitems, "pid not found in the list of running process");
	free(map);
	return ret;
error:
	if (map)
		free(map);
	return -1;
}
TCHAR *convert_dos_path(TCHAR * path)
{
	TCHAR *tmp = NULL;
	PTCHAR split[10];
	uint16_t i = 2;
	size_t size = 1;
	LPCTSTR lpDevicePath;
	TCHAR d = TEXT('A'), delimiter[5] = {TEXT("\\")};
	TCHAR szBuff[5];
	TCHAR driveletter[10],tmpdevice[100];
	TCHAR rawdrive[100];
	PTCHAR ret = NULL, devicepath = NULL;
	devicepath = (PTCHAR)calloc(1, sizeof(TCHAR));
	tmp = _tcstok(path, TEXT("\\"));
	split[0] = tmp;
	tmp = _tcstok(NULL, TEXT("\\"));
	split[1] = tmp;
	while (tmp)
	{
		tmp = _tcstok(NULL, TEXT("\\"));
		split[i++] = tmp;
		if(tmp)
		{
			size += (_tcslen(tmp) + 1);
			devicepath = (PTCHAR)realloc(devicepath, size * sizeof(TCHAR));
			_tcscat(devicepath, delimiter);
			_tcscat(devicepath, tmp);
		}
	} 
	check(_stprintf(rawdrive,_T("\\%hs\\%hs"), split[0], split[1]),"unable to join splited path");
	while (d <= TEXT('Z')) {
		TCHAR szDeviceName[3] = { d, TEXT(':'), TEXT('\0') };
		TCHAR szTarget[512] = { 0 };
		if (QueryDosDevice(szDeviceName, szTarget, 511) != 0) {
			if (_tcscmp(rawdrive, szTarget) == 0) {
				_stprintf_s(szBuff, _countof(szBuff), TEXT("%c:"), d);
				_tcscpy(driveletter, szBuff);
				break;
			}
		}
		d++;
	}
	ret = (TCHAR *)calloc(100, sizeof(TCHAR));
	check_mem(ret);
	check(_stprintf(ret,TEXT("%hs%hs"), driveletter,devicepath), "unable to join driveletter and rawdrive");
	return ret;
error:
	if (ret)
		free(ret);
	return NULL;
}
int
pid_in_pids(DWORD pid) {
	DWORD *proclist = NULL;
	DWORD numberOfReturnedPIDs;
	DWORD i;

	proclist = pids(&numberOfReturnedPIDs);
	if (proclist == NULL)
		return -1;
	for (i = 0; i < numberOfReturnedPIDs; i++) {
		if (proclist[i] == pid) {
			free(proclist);
			return 1;
		}
	}
	free(proclist);
	return 0;
}

int is_phandle_running(HANDLE hProcess, DWORD pid) {
	DWORD processExitCode = 0;
	LPTSTR err = NULL;
	if (hProcess == NULL) {
		if (GetLastError() == ERROR_INVALID_PARAMETER) {
			SetLastError(ERROR_NOSUCHPROCESS);
			return 0;
		}
		err = GetError();
		if (GetLastError() != ERROR_ACCESS_DENIED)
			log_err("%s", err);
		LocalFree(err);
		return -1;
	}

	if (GetExitCodeProcess(hProcess, &processExitCode)) {
		// XXX - maybe STILL_ACTIVE is not fully reliable as per:
		// http://stackoverflow.com/questions/1591342/#comment47830782_1591379
		if (processExitCode == STILL_ACTIVE) {
			return 1;
		}
		else {
			// We can't be sure so we look into pids.
			if (pid_in_pids(pid) == 1) {
				
				return 1;
			}
			else {
				CloseHandle(hProcess);
				return 0;
			}
		}
	}
	log_err("GetExitCodeProcess Failed");
	CloseHandle(hProcess);
	err = GetError();
	if(GetLastError() != ERROR_ACCESS_DENIED)
		log_err("%s", err);
	LocalFree(err);
	return -1;
}

HANDLE handle_from_pid_waccess(DWORD pid, DWORD dwDesiredAccess) {
	HANDLE hProcess;
	LPTSTR err = NULL;
	int ret;
	if (pid == 0) // otherwise we'd get NoSuchProcess
	{	
		SetLastError(ERROR_ACCESS_DENIED);
		return NULL;
	}
	hProcess = OpenProcess(dwDesiredAccess, FALSE, pid);
	ret = is_phandle_running(hProcess, pid);
	if (ret == 1)
		return hProcess;
	else if(ret == 0)
	{
		err = GetError();
		SetLastError(ERROR_NOSUCHPROCESS);
		goto error;
	}
	else
	{
		goto error;
	}
error:
	if (err)
		LocalFree(err);
	return NULL;
	
}

char *get_cmdline(long pid) {
	char *ret = NULL;
	int32_t pid_running;
	HANDLE hProcess = NULL;
	PVOID pebAddress;
	PVOID rtlUserProcParamsAddress;
	UNICODE_STRING commandLine;
	WCHAR *commandLineContents = NULL;
	static _NtQueryInformationProcess NtQueryInformationProcess = NULL;
	PROCESS_BASIC_INFORMATION pbi;
	LPTSTR err = NULL;
	if (pid == 0 || pid == 4)
	{
		return NULL;
	}
	pid_running = pid_is_running(pid);
	if ((pid_running == -1) || (pid_running == 0))
	{
		SetLastError(ERROR_NOSUCHPROCESS);
		return NULL;
	}
	hProcess = handle_from_pid_waccess(pid, PROCESS_QUERY_INFORMATION | PROCESS_VM_READ);
	if (hProcess == NULL)
		return NULL;
	NtQueryInformationProcess = (_NtQueryInformationProcess)GetProcAddress(
			GetModuleHandleA("ntdll.dll"), "NtQueryInformationProcess");

	NtQueryInformationProcess(hProcess, 0, &pbi, sizeof(pbi), NULL);
	pebAddress = pbi.PebBaseAddress;
	// get the address of ProcessParameters
#ifdef _WIN64
	check(ReadProcessMemory(hProcess, (PCHAR)pebAddress + 32,
		&rtlUserProcParamsAddress, sizeof(PVOID), NULL), "Could not read the address of ProcessParameters!\n");
#else
	check(ReadProcessMemory(hProcess, (PCHAR)pebAddress + 0x10,
		&rtlUserProcParamsAddress, sizeof(PVOID), NULL), "Could not read the address of ProcessParameters!\n");
#endif
	// read the CommandLine UNICODE_STRING structure
#ifdef _WIN64
	check(ReadProcessMemory(hProcess, (PCHAR)rtlUserProcParamsAddress + 112,
		&commandLine, sizeof(commandLine), NULL), "Could not to read Command Line Unicode Structure");
#else
	check(ReadProcessMemory(hProcess, (PCHAR)rtlUserProcParamsAddress + 0x40,
		&commandLine, sizeof(commandLine), NULL), "Could not read Command Line Unicode Structure");
#endif
	// allocate memory to hold the command line
	commandLineContents = (WCHAR *)malloc(commandLine.Length + 1);
	check_mem(commandLineContents);

	// read the command line
	check(ReadProcessMemory(hProcess, commandLine.Buffer,
		commandLineContents, commandLine.Length, NULL), "Failed to read the commandline");

	// Null-terminate the string to prevent wcslen from returning
	// incorrect length the length specifier is in characters, but
	// commandLine.Length is in bytes.
	commandLineContents[(commandLine.Length / sizeof(WCHAR))] = '\0';
	ret = ConvertWcharToChar(commandLineContents);
	/* TODO : attempt to parse the command line using Win32 API  and return list of commandline args*/
	free(commandLineContents);
	CloseHandle(hProcess);
	return ret;
error:
	err = GetError(); log_err("%s", err); LocalFree(err);
	return NULL;
}


char *exe(pid_t pid) {
	HANDLE hProcess;
	TCHAR *tmp;
	WCHAR tmpexe[MAX_PATH];
	TCHAR exe[MAX_PATH];
	LPTSTR err = NULL;
	char *ret = NULL;
	if (pid == 0 || pid == 4)
	{
		SetLastError(ERROR_ACCESS_DENIED);
		goto error;
	}
	else
	{
		hProcess = handle_from_pid_waccess(pid, PROCESS_QUERY_INFORMATION);
		check(hProcess, "NULL HANDLE");
		check(GetProcessImageFileName(hProcess, exe, MAX_PATH), "Unable to get process image file name");
		CloseHandle(hProcess);
	}
	tmp = convert_dos_path(exe);
	_swprintf(tmpexe,L"%hs",tmp);
	ret = ConvertWcharToChar(tmpexe);
	return ret;
error:
	err = GetError();
	log_err("%s", err);
	LocalFree(err);;
	if(hProcess)
		CloseHandle(hProcess);
	return NULL;
}
char *name(pid_t pid)
{
	/*"""Return process name, which on Windows is always the final
	part of the executable.
	"""*/
	// This is how PIDs 0 and 4 are always represented in taskmgr
	// and process - hacker.
	char *ret = NULL;
	TCHAR tmpret[_MAX_FNAME];
	HANDLE hProcess = NULL;
	bool fail = true;
	PROCESSENTRY32W pentry;
	HANDLE hSnapShot = INVALID_HANDLE_VALUE;
	ret = (char *)calloc(50, sizeof(char));
	check_mem(ret);
	if (pid == 0)
		strcpy(ret, "System Idle Process");
	else if (pid == 4)
		strcpy(ret, "System");
	else
	{
		free(ret);
		ret = NULL;
		// Note : this will fail with AD for most PIDs owned
		// by another user but it's faster.
		hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid); 
		if (!hProcess)
		{
			HMODULE hMod;
			DWORD cbNeeded;
			if (EnumProcessModules(hProcess, &hMod, sizeof(hMod),
				&cbNeeded))
			{
				if (GetModuleBaseName(hProcess, hMod, tmpret,
					sizeof(tmpret) / sizeof(TCHAR)))
				{
					fail = false;
					ret = ConvertWcharToChar((WCHAR *)tmpret);
					check(ret, "Converting WCHAR failed");
				}
			}
		}
		if(fail)
		{
			int ok;
			hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, pid);
			check(hSnapShot != INVALID_HANDLE_VALUE,"Error in creating HANDLE")
			pentry.dwSize = sizeof(PROCESSENTRY32W);
			ok = Process32FirstW(hSnapShot, &pentry);
			check(ok, "Error in retrieving information about the first process encountered in a system snapshot.");
			while (ok) {
				if (pentry.th32ProcessID == pid) {
					ret = ConvertWcharToChar(pentry.szExeFile);
					check(ret, "Converting WCHAR failed");
					fail = false;
					break;
				}
				ok = Process32NextW(hSnapShot, &pentry);
			}
			CloseHandle(hSnapShot);
		}
		if (fail)
		{
			SetLastError(ERROR_NOSUCHPROCESS);
			goto error;
		}
	}
	return ret;
error:
	if (hSnapShot)
		CloseHandle(hSnapShot);
	if (hProcess)
		CloseHandle(hProcess);
	if (ret)
		free(ret);
	return NULL;
}
/*
* Return process username as a "DOMAIN//USERNAME" string.
*/
char * Username(pid_t pid)
{
	HANDLE processHandle = NULL;
	HANDLE tokenHandle = NULL;
	PTOKEN_USER user = NULL;
	ULONG bufferSize;
	WCHAR *name = NULL;
	WCHAR *domainName = NULL;
	ULONG nameSize;
	ULONG domainNameSize;
	SID_NAME_USE nameUse;
	LPTSTR err = NULL;
	char *ret = NULL;
	// resolve the SID to a name
	nameSize = 0x100;
	domainNameSize = 0x100;
	// Get the user SID.
	bufferSize = 0x100; 
	ret = (char *)calloc(50, sizeof(char));
	if (pid == 0 || pid == 4)
		strcpy(ret, "NT AUTHORITY\\SYSTEM");
	else
	{
		processHandle = handle_from_pid_waccess(
			pid, PROCESS_QUERY_INFORMATION);
		check(processHandle, "Error in getting Handle from pid ");
		check(OpenProcessToken(processHandle, TOKEN_QUERY, &tokenHandle), "Error in Opening Process Token");
		CloseHandle(processHandle);
		processHandle = NULL;
		// Get the user SID.
		while (1) {
			user = malloc(bufferSize);
			check_mem(user);
			if (!GetTokenInformation(tokenHandle, TokenUser, user, bufferSize,
				&bufferSize))
			{
				check((GetLastError() == ERROR_INSUFFICIENT_BUFFER),"Error in retriving TokenUser using GetTokenInformation ");
				free(user);
				continue;
			}
			break;
		}
		CloseHandle(tokenHandle);
		tokenHandle = NULL;
		// resolve the SID to a name
		while (1) {
			name = malloc(nameSize * sizeof(WCHAR));
			check_mem(name);
			domainName = malloc(domainNameSize * sizeof(WCHAR));
			check_mem(domainName);
			if (!LookupAccountSidW(NULL, user->User.Sid, name, &nameSize,
				domainName, &domainNameSize, &nameUse))
			{
				check((GetLastError() == ERROR_INSUFFICIENT_BUFFER), "Error in getting name from SID");
				free(name);
				free(domainName);
				continue;
			}
			break;
		}
		ret = (char *)realloc(ret, (wcslen(domainName) + wcslen(name) + 5) * sizeof(char));
		sprintf(ret, "%s\\%s", ConvertWcharToChar(domainName), ConvertWcharToChar(name));
	}
	return ret;
error:
	err = GetError();
	log_err("%s", err);
	LocalFree(err);;
	if (processHandle != NULL)
		CloseHandle(processHandle);
	if (tokenHandle != NULL)
		CloseHandle(tokenHandle);
	if (name != NULL)
		free(name);
	if (domainName != NULL)
		free(domainName);
	if (user != NULL)
		free(user);
	return NULL;
}
int
get_proc_info(DWORD pid, PSYSTEM_PROCESS_INFORMATION *retProcess,
	PVOID *retBuffer) {
	static ULONG initialBufferSize = 0x4000;
	NTSTATUS status;
	PVOID buffer;
	ULONG bufferSize;
	PSYSTEM_PROCESS_INFORMATION process;

	// get NtQuerySystemInformation
	typedef DWORD(_stdcall * NTQSI_PROC) (int, PVOID, ULONG, PULONG);
	NTQSI_PROC NtQuerySystemInformation;
	HINSTANCE hNtDll;
	hNtDll = LoadLibrary(TEXT("ntdll.dll"));
	NtQuerySystemInformation = (NTQSI_PROC)GetProcAddress(
		hNtDll, "NtQuerySystemInformation");

	bufferSize = initialBufferSize;
	buffer = malloc(bufferSize);
	check_mem(buffer);

	while (TRUE) {
		status = NtQuerySystemInformation(SystemProcessInformation, buffer,
			bufferSize, &bufferSize);

		if (status == STATUS_BUFFER_TOO_SMALL ||
			status == STATUS_INFO_LENGTH_MISMATCH)
		{
			free(buffer);
			buffer = malloc(bufferSize);
			check_mem(buffer);

		}
		else {
			break;
		}
	}

	check((status == 0),"NtQuerySystemInformation() syscall failed");

	if (bufferSize <= 0x20000)
		initialBufferSize = bufferSize;

	process = PSUTIL_FIRST_PROCESS(buffer);
	do {
		if (process->UniqueProcessId == (HANDLE)pid) {
			*retProcess = process;
			*retBuffer = buffer;
			return 1;
		}
	} while ((process = PSUTIL_NEXT_PROCESS(process)));

	SetLastError(ERROR_NOSUCHPROCESS);
	goto error;

error:
	FreeLibrary(hNtDll);
	if (buffer != NULL)
		free(buffer);
	return 0;
}

enum proc_status status(pid_t pid) {
	ULONG i;
	bool suspended = true;
	PSYSTEM_PROCESS_INFORMATION process;
	PVOID buffer;
	LPTSTR err = NULL;
	check(get_proc_info(pid, &process, &buffer), "Cannot get Process Info");
	for (i = 0; i < process->NumberOfThreads; i++) {
		if (process->Threads[i].ThreadState != Waiting ||
			process->Threads[i].WaitReason != Suspended)
		{
			suspended = false;
			break;
		}
	}
	free(buffer);
	if (suspended)
		return STATUS_STOPPED;
	else
		return STATUS_RUNNING;
error:
	err = GetError();
	log_err("%s", err);
	LocalFree(err);;
	return 20;
}

/*
* Get process priority as a enum proc_priority.
*/
enum proc_priority nice(pid_t pid)
{
	DWORD priority;
	HANDLE hProcess;
	int32_t value;
	LPTSTR err = NULL;
	hProcess = handle_from_pid_waccess(pid, PROCESS_QUERY_INFORMATION | PROCESS_VM_READ);
	check(hProcess, "INVALID HANDLE");
	priority = GetPriorityClass(hProcess);
	check(priority != 0, "INVALID PRIORITY");
	CloseHandle(hProcess);
	return priority;

error:
	err = GetError();
	log_err("%s", err);
	LocalFree(err);
	if (hProcess)
		free(hProcess);
	return PRIORITY_ERROR;
}
/*
* Return a double indicating the process create time expressed in
* seconds since the epoch.
*/

double create_time(pid_t pid) {
	long long   unix_time = -1;
	HANDLE      hProcess;
	FILETIME    ftCreate, ftExit, ftKernel, ftUser;
	PSYSTEM_PROCESS_INFORMATION process;
	PVOID buffer = NULL;
	LPTSTR err = NULL;
	// special case for PIDs 0 and 4, return system boot time
	if (0 == pid || 4 == pid)
	{
		return get_boot_time();
	}
	hProcess = handle_from_pid_waccess(pid, PROCESS_QUERY_INFORMATION | PROCESS_VM_READ);
	if((hProcess == NULL) && ((GetLastError() == ERROR_ACCESS_DENIED) ||
		(errno == EPERM) || (errno == EACCES))){
		if (get_proc_info(pid, &process, &buffer))
		{
			goto find_time;
		}
	}
	check(hProcess," failed to get Process Handle");
	if (!GetProcessTimes(hProcess, &ftCreate, &ftExit, &ftKernel, &ftUser)) {	
		if ((GetLastError() == ERROR_ACCESS_DENIED) || 
			(errno == EPERM) || (errno == EACCES)) {
			if (get_proc_info(pid, &process, &buffer))
			{
				goto find_time;
			}
			else {
				// usually means the process has died so we throw a
				// NoSuchProcess here
				CloseHandle(hProcess);
				SetLastError(ERROR_NOSUCHPROCESS);
				return -1;
			}
		}
		else {
			
			goto error;
		}
	}

	CloseHandle(hProcess);

	// Convert the FILETIME structure to a Unix time.
	// It's the best I could find by googling and borrowing code here
	// and there. The time returned has a precision of 1 second.

	unix_time = ((LONGLONG)ftCreate.dwHighDateTime) << 32;
	unix_time += ftCreate.dwLowDateTime - 116444736000000000LL;
	unix_time /= 10000000;
	return ((double)unix_time);
find_time:
	unix_time = ((LONGLONG)process->CreateTime.HighPart) << 32;
	unix_time += process->CreateTime.LowPart - 116444736000000000LL;
	unix_time /= 10000000;
	return ((double)unix_time);
error:
	err = GetError();
	log_err("%s", err);
	LocalFree(err);
	if(hProcess)
		CloseHandle(hProcess);
	return -1;
}

/*
* Get various process information by using NtQuerySystemInformation.
* - TODO : user/kernel times 
* - TODO : io counters 
*/
Process *get_process(pid_t pid) {
	PSYSTEM_PROCESS_INFORMATION process;
	PVOID buffer;
	ULONG num_handles;
	ULONG i;
	ULONG ctx_switches = 0;
	long long create_time;
	int num_threads;
	enum proc_status _status = STATUS_STOPPED;
	Process *process_info = NULL;
	check(get_proc_info(pid, &process, &buffer),"error in getting process info");
	num_handles = process->HandleCount;
	for (i = 0; i < process->NumberOfThreads; i++)
		ctx_switches += process->Threads[i].ContextSwitches;
	// Convert the LARGE_INTEGER union to a Unix time.
	// It's the best I could find by googling and borrowing code here
	// and there. The time returned has a precision of 1 second.
	if (0 == pid || 4 == pid) {
		create_time = get_boot_time(); 
	}
	else {
		create_time = ((LONGLONG)process->CreateTime.HighPart) << 32;
		create_time += process->CreateTime.LowPart - 116444736000000000LL;
		create_time /= 10000000;
	}
	for (i = 0; i < process->NumberOfThreads; i++) {
		if (process->Threads[i].ThreadState != Waiting ||
			process->Threads[i].WaitReason != Suspended)
		{
			_status = STATUS_RUNNING;
			break;
		}
	}
	num_threads = (int)process->NumberOfThreads;
	free(buffer);
	process_info = (Process *)calloc(1, sizeof(Process));
	check_mem(process_info);
	process_info->pid = pid;
	process_info->ppid = ppid(pid);
	process_info->name = name(pid);
	process_info->exe = exe(pid);
	process_info->username = Username(pid);
	process_info->nice = nice(pid);
	process_info->cmdline = get_cmdline(pid);
	process_info->num_handles = num_handles;
	process_info->num_ctx_switches = ctx_switches;
	process_info->create_time = (double)create_time;
	process_info->num_threads = num_threads;
	process_info->status = _status;
	return process_info;
error:
	return NULL;
}
void free_process(Process *p)
{
	if (p)
	{
		free(p->name);
		free(p->exe);
		free(p->username);
		free(p->cmdline);
		free(p);
	}

}
/*
The following function is an ugly workaround to ensure that coverage
data can be manually flushed to disk during py.test invocations. If
this is not done, we cannot measure the coverage information. More details
in http://... link_to_bug...

*/
void gcov_flush(void) { __gcov_flush(); }

