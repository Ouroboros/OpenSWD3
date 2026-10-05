#pragma once

#include <filesystem>

namespace openswd3::resource_io {

// sub_409C10: visit Save/0.sav through Save/98.sav in ascending order and
// return whether at least one file can be opened for reading and writing.
[[nodiscard]] bool
scan_legacy_save_slots(const std::filesystem::path& data_directory);

}  // namespace openswd3::resource_io
