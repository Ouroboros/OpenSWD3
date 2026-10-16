#pragma once

#include <cstdint>

// Menu helpers: list walking, cursor movement, boxes and item names.
namespace swd3 {

struct ActState;

int DrawActGhost(ActState* state, int x, int y, int alpha);
int CountList(std::uint8_t** list);
void ListNodeAtInto(int index, std::uint8_t** list, std::uint8_t** node);
std::uint8_t* ListNodeAt(int index, std::uint8_t** list);
void LoadItemName(const std::uint8_t* item);
void ReadMenuMouseButtons();
void DrawMenuBox(int x, int y, int width, int height, int mode, int red, int green, int blue);
void ClampMenuCursor(const int* total, int* first, int* cursor, const int* rows);
void MenuCursorDown(const int* total, int* first, int* cursor, const int* rows);
void MenuCursorUp(int unused, int* first, int* cursor);
void MenuPageDown(const int* total, int* first, int* cursor, int* rows, int page);
void MenuPageUp(int unused, int* first, int* cursor, int unused2, int page);
void CountListUpTo(std::uint8_t* list, int* count, int limit);

}  // namespace swd3
