
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
