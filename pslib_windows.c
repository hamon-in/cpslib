
#include <windows.h>
#include <wtsapi32.h>
#include <stdio.h>
#include <winternl.h>

#include "pslib.h"
#include "common.h"
#include "pslib_windows.h"

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
