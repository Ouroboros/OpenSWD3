#include "battle/battle_formation.h"

#include "pic/projector.h"
#include "swd3/w32.h"

namespace swd3 {

// swd32.exe 0x0045F0F0
BattleFormation::BattleFormation(Projector* projector) {
    at<Projector*>(0) = projector;
    int step = 0;
    for (int value = 0; value < 0x96; value += 5) {
        at<std::int32_t>(0x3104 + 8 * step) = step;
        at<std::int32_t>(0x3108 + 8 * step) = value / 4;
        ++step;
    }
}

// swd32.exe 0x0045F130: reads the index of `fileName`; *counts points at the formation counts per group
int BattleFormation::LoadIndex(const char* fileName, std::uint8_t** counts) {
    w32::DWORD read = 0;
    w32::HANDLE file = w32::CreateFileA(fileName, w32::GENERIC_READ, 0, nullptr, w32::OPEN_EXISTING,
                                        w32::FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == w32::INVALID_HANDLE_VALUE) {
        w32::CloseHandle(w32::INVALID_HANDLE_VALUE);
        return 0;
    }
    w32::ReadFile(file, m_data + 4, 0x2714, &read, nullptr);
    *counts = m_data + 0x1F48;
    w32::CloseHandle(file);
    return 1;
}

// swd32.exe 0x0045F1B0: reads formation `index` (1-based) of `group` (0x10C bytes) into `formation`; groups count
// their formations in signed bytes from group 1
int BattleFormation::LoadFormation(const char* fileName, void* formation, std::uint16_t group, std::int8_t index) {
    w32::DWORD read = 0;
    w32::HANDLE file = w32::CreateFileA(fileName, w32::GENERIC_READ, 0, nullptr, w32::OPEN_EXISTING,
                                        w32::FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == w32::INVALID_HANDLE_VALUE) {
        w32::CloseHandle(w32::INVALID_HANDLE_VALUE);
        return 0;
    }
    w32::ReadFile(file, m_data + 4, 0x2714, &read, nullptr);
    const auto count = static_cast<std::int8_t>(m_data[group + 0x1F48]);
    if (count <= 0 || index > count) {
        w32::CloseHandle(file);
        return 0;
    }
    int before = 0;
    for (int previous = 1; previous < group; ++previous) {
        before += static_cast<std::int8_t>(m_data[previous + 0x1F48]);
    }
    const std::int32_t number = at<std::int32_t>(8 + 4 * (before + index));
    w32::SetFilePointer(file, 0x10C * number + 0x2714, nullptr, w32::FILE_BEGIN);
    w32::ReadFile(file, formation, 0x10C, &read, nullptr);
    w32::CloseHandle(file);
    return 1;
}

}  // namespace swd3
