#pragma once

#include <vector>

#include "core/types.h"

namespace swd3
{

// The letters of the CD-ROM drives, a to z (GetDriveTypeA == DRIVE_CDROM); none where drive letters do not exist.
std::vector<char> CdRomDrives();
// GlobalMemoryStatus().dwTotalPhys: the physical memory in bytes, at most 0xFFFFFFFF.
u32 TotalPhysicalMemory();
// timeGetTime(): milliseconds since the system started.
u32 SystemUptime();

// GetLocalTime(): the local date and time to the minute.
struct LocalDateTime
{
    int year = 0;
    int month = 0;
    int day = 0;
    int hour = 0;
    int minute = 0;
};

LocalDateTime LocalTime();
// The id of this process.
u32 ProcessId();
// The processor faults that end the process logged first (an unhandled structured exception on Windows, a fault
// signal elsewhere).
void InstallFaultLog();
// Sleep(): the calling thread waits.
void SleepMilliseconds(u32 milliseconds);

}  // namespace swd3
