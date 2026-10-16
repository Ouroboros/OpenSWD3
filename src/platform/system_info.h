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

}  // namespace swd3
