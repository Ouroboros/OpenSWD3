#pragma once

#include <string_view>

#include "core/types.h"

namespace swd3
{

class ErrorReporter;

// The maintenance assertion box: the message formatted with `value`, the source file and line. Abort and Retry end
// the game (the original closes the window and breaks into the debugger, which ends a process run without one).
void MaintenanceAssert(ErrorReporter& reporter, std::u8string_view message, i32 value, std::string_view file, u32 line);

}  // namespace swd3
