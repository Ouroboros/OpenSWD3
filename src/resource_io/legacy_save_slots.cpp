#include "openswd3/resource_io/legacy_save_slots.hpp"

#include <fstream>
#include <string>

namespace openswd3::resource_io {

bool scan_legacy_save_slots(const std::filesystem::path& data_directory) {
    bool found = false;
    for (int slot = 0; slot <= 98; ++slot) {
        const std::filesystem::path path =
            data_directory / "Save" / (std::to_string(slot) + ".sav");
        std::fstream file(
            path, std::ios::in | std::ios::out | std::ios::binary
        );
        if (file.is_open()) {
            found = true;
        }
    }

    return found;
}

}  // namespace openswd3::resource_io
