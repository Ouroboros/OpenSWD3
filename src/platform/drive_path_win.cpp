#include "platform/file_system.h"

namespace swd3
{

std::string DrivePath(std::string_view gamePath)
{
    return std::string(gamePath);
}

}  // namespace swd3
