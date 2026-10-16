#pragma once

#include <cstdint>

namespace swd3 {

// Clickable map region (list at G.clickRegions).
struct ClickRegion {
    std::int32_t unknown0;
    ClickRegion* next;     // +0x04
    std::uint16_t left;    // +0x08
    std::uint16_t top;     // +0x0A
    std::uint16_t right;   // +0x0C
    std::uint16_t bottom;  // +0x0E
};

// Event list node (list at G.eventList): next, id, ...
struct EventNode {
    EventNode* next;
    std::int32_t id;
};

int CountClickRegions();
int FindClickRegion(ClickRegion** found);
int FreeClickRegions();
std::uint8_t* ResetMoveRecord(std::uint8_t* record);
EventNode* FindEvent(int id);
bool GameFlag(int flag);
std::uint8_t SetGameFlag(int flag);
std::uint8_t ClearGameFlag(int flag);
std::uint8_t SetGameFlagTo(int flag, int value);
int SetWalkSpeed(int speed);
int SetInputLock(int value);
int ClearInputLock();
std::uint32_t ClearFollower(void* record);
std::uint8_t LoadRoleRecord(std::uint8_t* role, const std::uint8_t* record);
void DrawMarchingRect(std::uint16_t* pixels, int x, int y, int width, int height, int pitch);
int DirectionToPlayer(int x, int y);
std::uint16_t* ShrinkScreenQuarter(std::uint16_t* pixels);
int AngleBetween(int fromX, int fromY, int toX, int toY, std::int32_t* distance);
int AngleToDirection(int angle);

}  // namespace swd3
