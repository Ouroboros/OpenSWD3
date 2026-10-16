#include "core/fault.h"

#include <cstdlib>

#include "platform/message_box.h"
#include "platform/system_info.h"

namespace swd3
{

void Fault(ErrorReporter& reporter, std::u8string_view what)
{
    reporter.ShowError(u8"swd32.exe", what);
    std::abort();
}

namespace
{

constexpr int kHangPause = 1000;  // milliseconds slept per round of an emulated hang

}  // namespace

void Hang()
{
    for (;;)
    {
        SleepMilliseconds(kHangPause);
    }
}

}  // namespace swd3
