#include "core/fault.h"

#include <cstdlib>

#include "core/log.h"
#include "platform/message_box.h"
#include "platform/system_info.h"

namespace swd3
{

void Fault(ErrorReporter& reporter, std::u8string_view what)
{
    LogError(u8"processor fault of the original: %s", what);
    reporter.ShowError(u8"swd32.exe", what);
    std::abort();
}

namespace
{

constexpr int kHangPause = 1000;  // milliseconds slept per round of an emulated hang

}  // namespace

void Hang()
{
    LogError(u8"a loop of the original that never ends: the game hangs");

    for (;;)
    {
        SleepMilliseconds(kHangPause);
    }
}

}  // namespace swd3
