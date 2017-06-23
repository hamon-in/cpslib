
#include <windows.h>
#include <wtsapi32.h>
#include <stdio.h>
#include <winternl.h>

#include "pslib.h"
#include "common.h"

#pragma comment(lib, "WTSAPI32.lib")
// fix for mingw32, see
// https://github.com/giampaolo/psutil/issues/351#c2
typedef struct _DISK_PERFORMANCE_WIN_2008 {
	LARGE_INTEGER BytesRead;
	LARGE_INTEGER BytesWritten;
	LARGE_INTEGER ReadTime;
	LARGE_INTEGER WriteTime;
	LARGE_INTEGER IdleTime;
	DWORD         ReadCount;
	DWORD         WriteCount;
	DWORD         QueueDepth;
	DWORD         SplitCount;
	LARGE_INTEGER QueryTime;
	DWORD         StorageDeviceNumber;
	WCHAR         StorageManagerName[8];
} DISK_PERFORMANCE_WIN_2008;

typedef struct _WINSTATION_INFO {
	BYTE Reserved1[72];
	ULONG SessionId;
	BYTE Reserved2[4];
	FILETIME ConnectTime;
	FILETIME DisconnectTime;
	FILETIME LastInputTime;
	FILETIME LoginTime;
	BYTE Reserved3[1096];
	FILETIME CurrentTime;
} WINSTATION_INFO;


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
