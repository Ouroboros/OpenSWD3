#pragma once

#include <atomic>
#include <thread>

#include "core/types.h"

namespace swd3
{

// GlobalMemoryStatus().dwTotalPhys: the physical memory in bytes, at most 0xFFFFFFFF.
u32 TotalPhysicalMemory();
// timeGetTime(): milliseconds since the system started.
u32 SystemUptime();
// timeBeginPeriod(1): SystemUptime and the waits at a millisecond (the default of the systems the game ran on; a
// newer Windows ticks at 15.6 ms, which spaced the 35 ms frames 47 ms apart). Nothing elsewhere.
void UseFineTimer();

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

// A watch of the thread that made it (the main loop): when Beat() has not been called for StallSeconds, the thread's
// call stack is logged (with the PDB's symbols on Windows), and again every RepeatSeconds while it stays stalled.
// Nothing on other platforms.
class HangWatch
{
public:
    static constexpr u32 StallSeconds = 5;
    static constexpr u32 RepeatSeconds = 30;

    HangWatch();
    ~HangWatch();
    HangWatch(const HangWatch&) = delete;
    HangWatch& operator=(const HangWatch&) = delete;

    void Beat();

private:
    void Watch();

    std::atomic<u32> beats{0};
    std::atomic<bool> stopping{false};
    [[maybe_unused]] void* watched = nullptr;  // the watched thread's handle (Windows)
    std::thread thread;
};

}  // namespace swd3
