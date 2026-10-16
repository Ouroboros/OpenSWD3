#pragma once

#include <cstdint>

// Counter lists (0x14 byte nodes: next, id, value, percent, done) of fame, skills and the like, and the three fame
// lists of the save data.
namespace swd3 {

void CloseRecordFiles();
int ClearCounterList(std::uint32_t* list);
std::uint16_t AddCounterCapped(std::uint8_t* list, std::int16_t id, std::int16_t amount);
std::uint16_t SetCounterCapped(std::uint8_t* list, std::int16_t id, std::uint16_t value);
std::int16_t FindCounterValue(std::uint8_t* list, std::int16_t id);
int StepCounterProgress(std::uint8_t* list, std::int16_t id, std::uint16_t maximum, std::uint16_t scale);
void SetCounterProgress(std::uint8_t* list, std::int16_t id, std::uint16_t maximum, std::uint16_t value);
std::int16_t FindCounterPercent(std::uint8_t* list, std::int16_t id);
int LoadFame(const std::uint8_t* block);
void FreeFame();

}  // namespace swd3
