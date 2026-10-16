#include "core/fault.h"

#include <cstdlib>

#include "platform/message_box.h"
#include "platform/system_info.h"

namespace swd3
{

void Fault(ErrorReporter& reporter, std::string_view what)
{
    reporter.ShowError("swd32.exe", what);
    std::abort();
}

void Hang()
{
    for (;;)
    {
        SleepMilliseconds(1000);
    }
}

}  // namespace swd3
