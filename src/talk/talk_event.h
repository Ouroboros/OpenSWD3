#pragma once

#include <string_view>

namespace swd3
{

struct Role;

// The failed run of a role's act, as the event commands report it.
void LogActError(std::string_view where, const Role& role);

}  // namespace swd3
