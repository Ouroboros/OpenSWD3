#pragma once

#include <string>

namespace swd3
{

// The launcher's shell commands: the original runs the browser or text viewer registered in HKEY_CLASSES_ROOT with
// the address or path appended; the system's default handler is asked instead.
void OpenWebsite(const std::string& address);
void OpenTextFile(const std::string& path);

}  // namespace swd3
