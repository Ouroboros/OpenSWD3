#pragma once

#include <string_view>

namespace swd3
{

class ErrorReporter;

// A processor fault the original does not handle (an integer division by zero): the process ends after the report
// (the original ends with the system's fault dialog).
[[noreturn]] void Fault(ErrorReporter& reporter, std::string_view what);

// A loop of the original that never ends: the caller does not return (without the undefined behaviour of an empty
// endless loop).
[[noreturn]] void Hang();

}  // namespace swd3
