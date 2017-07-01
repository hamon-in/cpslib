
#include <windows.h>
#include <wtsapi32.h>
#include <stdio.h>
#include <winternl.h>
#include <powrprof.h>
#include<Winsock2.h>
#include<ws2ipdef.h>
#include<Iphlpapi.h>
#if (_WIN32_WINNT >= 0x0600) // Windows Vista and above
#include <ws2tcpip.h>
#endif
#include "pslib.h"
#include "common.h"
#include "pslib_windows.h"
#pragma comment(lib, "IPHLPAPI.lib")
#pragma comment(lib, "WTSAPI32.lib")



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

		if (hDevice ==INVALID_HANDLE_VALUE)
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
	{
		free_disk_iocounter_info(ret);
	}
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
	{
		free(drive_letter);
	}
	if (ret)
	{
		free_disk_partition_info(ret);
	}
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
			return TRUE;
		}
		else
		{
			return FALSE;
		}
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
	char *tmp;
	PWINSTATIONQUERYINFORMATIONW WinStationQueryInformationW;
	WINSTATION_INFO station_info;
	HINSTANCE hInstWinSta = NULL;
	ULONG returnLen;
	LPBOOL q = NULL;

	UsersInfo *users_info_ptr = NULL;
	/*PyObject *py_retlist = PyList_New(0);
	PyObject *py_tuple = NULL;
	PyObject *py_address = NULL;
	PyObject *py_username = NULL;*/
	hInstWinSta = LoadLibraryA("winsta.dll");
	WinStationQueryInformationW = (PWINSTATIONQUERYINFORMATIONW) \
		GetProcAddress(hInstWinSta, "WinStationQueryInformationW");
	users_info_ptr = (UsersInfo *)calloc(1, sizeof(UsersInfo));
	check_mem(users_info_ptr);
	check(WTSEnumerateSessions(hServer, 0, 1, &sessions, &count),
			"error in retrieving list of sessions on a Remote Desktop Session Host server");
	users_info_ptr->users = (Users *)calloc(count, sizeof(Users));
	check_mem(users_info_ptr->users);
	//users_info_ptr->nitems = 0;
	users_info_ptr->nitems = 0;
	for (i = 0; i < count; i++) {
		//py_address = NULL;
		//py_tuple = NULL;
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
		tmp = (char *)calloc(USERNAME_LENGTH, sizeof(char));
		check_mem(tmp);
		if (WideCharToMultiByte(CP_ACP, WC_NO_BEST_FIT_CHARS, buffer_user, -1, tmp, USERNAME_LENGTH, "", q) == 0)
		{
			DWORD t;
			t = GetLastError();
			check(t != ERROR_INSUFFICIENT_BUFFER, "Supplied buffer size was not large enough, or it was incorrectly set to NULL.");
			check(t != ERROR_INVALID_FLAGS, "The values supplied for flags were not valid.");
			check(t != ERROR_INVALID_PARAMETER, "Any of the parameter values was invalid");
			check(t != ERROR_NO_UNICODE_TRANSLATION, "Invalid Unicode was found in a string.");
		}
		users_info_ptr->users[users_info_ptr->nitems].username = tmp;
		users_info_ptr->users[users_info_ptr->nitems].tstamp = (float)unix_time;
		users_info_ptr->users[users_info_ptr->nitems].tty = NULL;
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
	ret = (CpuTimes *)calloc(1, sizeof(CpuTimes));
	check_mem(ret);
	all_delta = (t2->user + t2->system + t2->interrupt + t2->idle + t2->dpc) - \
		(t1->user + t1->system + t1->interrupt + t1->idle + t1->dpc);
	if (all_delta == 0)
	{
		all_delta = 100;
	}
	ret->user = get_valid_value((t2->user - t1->user)*100.0 / all_delta);
	ret->system = get_valid_value((t2->system - t1->system)*100.0 / all_delta);
	ret->interrupt = get_valid_value((t2->interrupt - t1->interrupt)*100.0 / all_delta);
	ret->idle = get_valid_value((t2->idle - t1->idle)*100.0 / all_delta);
	ret->dpc = get_valid_value((t2->dpc - t1->dpc)*100.0 / all_delta);
	return ret;
error:
	if (ret)
	{
		free(ret);
	}
	return NULL;
}

/*"""Same as cpu_percent() but provides utilization percentages
for each specific CPU time as is returned by cpu_times().

*interval* and *percpu* arguments have the same meaning as in
cpu_percent().
"""*/

CpuTimes *cpu_times_percent(bool percpu, CpuTimes * Last_Cpu_times)
{
	uint32_t no_cpu, i;
	CpuTimes *t2 = NULL, *ret = NULL, *tmp = NULL;
	t2 = (CpuTimes *)calloc(1, sizeof(CpuTimes));
	check_mem(t2);
	if (!percpu) {
		t2 = cpu_times(false);
		ret = calculate(t2, Last_Cpu_times);
	}

	//		# per - cpu usage
	else {
		ret = (CpuTimes *)calloc(1, sizeof(CpuTimes));
		check_mem(ret);
		t2 = cpu_times(true);

		no_cpu = cpu_count(true);
		//t2, t2 in zip(tot2, _last_per_cpu_times_2) :
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

bool virtual_memory(VirtMemInfo *ret)
{
	MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);

    if (! GlobalMemoryStatusEx(&memInfo))
        return false;
            
    ret->total = memInfo.ullTotalPhys;
    ret->avail = memInfo.ullAvailPhys;
    ret->used = ret->total - ret->avail;
	ret->percent = (ret->used)*100.0/ret->total;
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
                buffer = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION)malloc(
                    length);
                check(buffer,"");
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
    
	
	CpuTimes *ret = (CpuTimes*)calloc(si.dwNumberOfProcessors,sizeof(CpuTimes));
	CpuTimes *c=ret;
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
	UINT i;
	CpuTimes *ret = (CpuTimes*)calloc(1,sizeof(CpuTimes));
	check_mem(ret);
	float user,system,idle,dpc,interrupt;
	idle = user = system = interrupt = dpc = 0;
	CpuTimes *c = per_cpu;
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

    check(GetSystemTimes(&idle_time, &kernel_time, &user_time),"")

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
    CpuTimes *data = (CpuTimes*)calloc(1,sizeof(CpuTimes));
    check_mem(data);
    data->idle=idle;
    data->system=system;
    data->user=user;
    
    CpuTimes *percpu = per_cpu_times();
    check_mem(percpu);
    CpuTimes *percpu_summed = sum_per_cpu_times(percpu);
    check_mem(percpu_summed);
    free(percpu);
	data->interrupt = percpu_summed->interrupt;
    data->dpc= percpu_summed->dpc;
    free(percpu_summed);
    return data;
error:
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

CpuStats *cpu_stats() {
    // NtQuerySystemInformation stuff
    typedef DWORD (_stdcall * NTQSI_PROC) (int, PVOID, ULONG, PULONG);
    NTQSI_PROC NtQuerySystemInformation;
    HINSTANCE hNtDll;

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
    InterruptInformation = \
        malloc(sizeof(_SYSTEM_INTERRUPT_INFORMATION) *
               si.dwNumberOfProcessors);
   
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
    
    
    CpuStats *ret = (CpuStats*)calloc(1,sizeof(CpuStats));
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

uint32_t *pids(DWORD *numberOfReturnedPIDs) {


    DWORD *procArray = NULL;
    DWORD procArrayByteSz;
    int procArraySz = 0;

    // Stores the byte size of the returned array from enumprocesses
    DWORD enumReturnSz = 0;

    do {
        procArraySz += 1024;
        free(procArray);
        procArrayByteSz = procArraySz * sizeof(DWORD);
        procArray = malloc(procArrayByteSz);
        
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
        //PyErr_SetFromWindowsErr(0);
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

    //PyErr_SetFromWindowsErr(0);
    CloseHandle(hProcess);
    return -1;
}

bool pid_exists(long pid) 
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
        if (pAddresses == NULL) {
            //PyErr_NoMemory();
            return NULL;
        }

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

    if (dwRetVal != NO_ERROR) {
        //PyErr_SetString(PyExc_RuntimeError, "GetAdaptersAddresses() syscall failed.");
        return NULL;
    }

    return pAddresses;
}


NetIOCounterInfo *net_io_counters_per_nic() 
{
    DWORD dwRetVal = 0;

#if (_WIN32_WINNT >= 0x0600) // Windows Vista and above
    MIB_IF_ROW2 *pIfRow = NULL;
#else // Windows XP
    MIB_IFROW *pIfRow = NULL;
#endif

    PIP_ADAPTER_ADDRESSES pAddresses = NULL;
    PIP_ADAPTER_ADDRESSES pCurrAddresses = NULL;
    //PyObject *py_retdict = PyDict_New();
    ///PyObject *py_nic_info = NULL;
    //PyObject *py_nic_name = NULL;

    //if (py_retdict == NULL)
    //    return NULL;
    pAddresses = get_nic_addresses();
    if (pAddresses == NULL)
        goto error;
    pCurrAddresses = pAddresses;

    
    NetIOCounterInfo *ret = (NetIOCounterInfo*) calloc(1,sizeof(NetIOCounterInfo));
    NetIOCounters *counters = (NetIOCounters*) calloc(15,sizeof(NetIOCounters));
    NetIOCounters *nc = counters;
    //check_mem(counters);
    //check_mem(ret);
    ret->nitems = 0;
    ret->iocounters = counters;

    while (pCurrAddresses) {
      //py_nic_name = NULL;
      //py_nic_info = NULL;

#if (_WIN32_WINNT >= 0x0600) // Windows Vista and above
      pIfRow = (MIB_IF_ROW2 *) malloc(sizeof(MIB_IF_ROW2));
#else // Windows XP
        pIfRow = (MIB_IFROW *) malloc(sizeof(MIB_IFROW));
#endif
	
        if (pIfRow == NULL) {
	  //PyErr_NoMemory();
	  goto error;
        }

#if (_WIN32_WINNT >= 0x0600) // Windows Vista and above
        SecureZeroMemory((PVOID)pIfRow, sizeof(MIB_IF_ROW2));
        pIfRow->InterfaceIndex = pCurrAddresses->IfIndex;
        dwRetVal = GetIfEntry2(pIfRow);
#else // Windows XP
        pIfRow->dwIndex = pCurrAddresses->IfIndex;
        dwRetVal = GetIfEntry(pIfRow);
#endif
	
        if (dwRetVal != NO_ERROR) {
	  //PyErr_SetString(PyExc_RuntimeError,"GetIfEntry() or GetIfEntry2() syscalls failed.");
	  goto error;
        }
	
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
	LPBOOL q = NULL;
	int len;
	len = WideCharToMultiByte(CP_ACP, WC_NO_BEST_FIT_CHARS, pCurrAddresses->FriendlyName, wcslen(pCurrAddresses->FriendlyName), nc->name, wcslen(pCurrAddresses->FriendlyName), "", q);
	pCurrAddresses->FriendlyName[len] = '\0';
	if ( len == 0)
	{
		DWORD t;
		t = GetLastError();
		//check(t != ERROR_INSUFFICIENT_BUFFER, "Supplied buffer size was not large enough, or it was incorrectly set to NULL.");
		//check(t != ERROR_INVALID_FLAGS, "The values supplied for flags were not valid.");
		//check(t != ERROR_INVALID_PARAMETER, "Any of the parameter values was invalid");
		//check(t != ERROR_NO_UNICODE_TRANSLATION, "Invalid Unicode was found in a string.");
	}

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
void free_netiocounterinfo(NetIOCounterInfo *ret)
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
	NetIOCounterInfo *sum=(NetIOCounterInfo*) calloc(1,sizeof(NetIOCounterInfo));
	NetIOCounters *r = (NetIOCounters*) calloc(1,sizeof(NetIOCounters));
	check(sum);
	check(r);
	sum->iocounters=r;
	sum->nitems=1;

	int i;
	
	NetIOCounters *c=info->iocounters;
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
	free_netiocounterinfo(info);
	return sum;
}
NetIOCounterInfo *net_io_counters(bool per_nic)
{
	NetIOCounterInfo *ret = net_io_counters_per_nic();
	if(per_nic)
		return ret;
	else
		return net_io_counters_summed(ret);
}

/*
void test_netiocounters()
{
  NetIOCounterInfo *c = net_io_counters(true);
  NetIOCounters *r = c->iocounters;
  
  if(!c)
  {
  	printf("Aborting..\n");
  	return;
  }
  int i=0;
  printf("__________NET IO COUNTERS_________\n");
  for(i=0;i<c->nitems;++i)
    {
 		printf("%s:\n",r->name);
    	printf("\tBytes Sent : %" PRIu64"\n",r->bytes_sent);
	  	printf("\tBytes Received : %"PRIu64"\n",r->bytes_recv);
	  	printf("\tPackets Sent : %"PRIu64"\n",r->packets_sent);
	  	printf("\tPackets Received : %" PRIu64"\n",r->packets_recv);
	  	printf("\tIn Errors : %" PRIu64"\n",r->errin);
	  	printf("\tOut Errors : %" PRIu64"\n",r->errout);
	  	printf("\tIn Discards : %" PRIu64"\n",r->dropin);
	  	printf("\tOut Discards : %" PRIu64"\n",r->dropout);
      	++r;
    }
    free_netiocounterinfo(c);
  
}
void test_virtual_memory()
{
	VirtMemInfo virt;
	if(!virtual_memory(&virt))
	{
		printf("Aborting\n");
		return;
	}
	printf("_______Virtual Memory_______\n");
	printf("Total=%lld, Used=%lld, Free=%lld, Percent=%f\n\n",virt.total,virt.used, virt.avail,virt.percent);
}
void test_swap_memory()
{
	SwapMemInfo swap;
	if(!swap_memory(&swap))
	{
		printf("Aborting\n");
		return;
	}
	printf("________Swap Memory________\n");
	printf("Total=%llu, Used=%llu, Free=%llu, Percent=%f, sin=%llu, sout=%llu\n\n",swap.total,swap.used, swap.free,swap.percent,swap.sin,swap.sout);
	
	
}
void test_pids()
{
	DWORD numberOfPIDs;
	DWORD *pid = pids(&numberOfPIDs);
	DWORD *c=pid;
	DWORD i;
	if(!pid)
	{
		printf("Aborting...\n");
		return;
	}
	printf("__________PIDs_________\n");
	for(i=0;i<numberOfPIDs;++i)
	{
		printf("%d, ",*c);
		++c;
	}
	printf("\n\n");
	free(pid);
}
void test_cpu_stats()
{
	CpuStats *tmp= cpu_stats();
	if(!tmp)
	{
		printf("Aborting...\n");
		return;
	}
	printf("_________Cpu Stats_________\n");
	printf("Context Switches = %lu \n", tmp->ctx_switches);
	printf("Interrupts = %lu \n", tmp->interrupts);
	printf("Soft Interrupts = %lu \n", tmp->soft_interrupts);
	printf("System Calls = %lu \n", tmp->syscalls);
	printf("Dpcs = %lu \n\n", tmp->dpcs);
	free(tmp);
}
void test_cputimes()
{
	CpuTimes * data = cpu_times(false);
	if(!data)
	{
		printf("Aborting..\n");
		return;
	}
	printf("_________CPU Times_________\n");
	printf("User=%f, Idle=%f, System=%f, Interrupt=%f, Dpc=%f\n\n",data->user,data->idle,data->system,data->interrupt,data->dpc);
	free(data);
}
void test_percputimes()
{
	CpuTimes * r = cpu_times(true);
	if(!r)
	{
		printf("Aborting..\n");
		return;
	}
	printf("_______CPU Times_Per CPU_______\n");
	CpuTimes * c=r;
	UINT i;
	for(i=0;i<cpu_count(true);++i)
	{
		printf("User=%f, Idle=%f, System=%f, Interrupt=%f, Dpc=%f\n",c->user,c->idle,c->system,c->interrupt,c->dpc);
		++c;		
	}
	printf("\n");
	free(r);
}
void test_boottime()
{
	uint32_t time = get_boot_time();
	printf("_________Boot Time_________\n");
	printf("%"PRIu32"\n\n", time);
}
void test_pid_exists()
{
	long pid=0;int i =0;
	printf("_________PID Exists_________\n");
	
	for (pid=rand()%22000,i=0;i<20;++i,pid=rand()%22000)
	{
		int x =0;
		x = pid_exists(pid);
		printf("%ld - %s \n",pid,x?"true":"false");
		
	}
}

void test_cpucount()
{
	printf("_________CPU Count_________\n");
	printf("Number of Logical Processors: %d\n",cpu_count(true));
	printf("Number of Physical Processors: %d\n\n",cpu_count(false));
}
void main()
{
	test_virtual_memory();
	test_swap_memory();
	test_cpucount();
	test_cputimes();
	test_percputimes();
	test_boottime();
	test_cpu_stats();
	test_pids();
	test_pid_exists();
	test_netiocounters();
}
*/
