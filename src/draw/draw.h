#pragma once

#include <cstdint>

#include "swd3/ddraw.h"

namespace swd3 {

// Blit routine of the per-mode tables (G.drawRleTable / G.drawRawTable).
using BlitFn = int(int x, int y, int width, int height, int arg);

int DrawInit(w32::IDirectDrawSurface4* surface);
void DrawTableInit();
std::uint8_t* LockSurface(w32::IDirectDrawSurface4* surface);
w32::HRESULT UnlockSurface(w32::IDirectDrawSurface4* surface, void* memory);
int SetClipRect(const w32::RECT* rect);
int SetClipRectClamped(int left, int top, int right, int bottom);
void DrawOutline(int x, int y, int width, int height, int flags, int arg);
int DrawPicture(int x, int y, int width, int height, int flags, int arg);
int DrawTile16(int x, int y);
void DrawTile16Keyed(int x, int y);
void DrawTile8(int x, int y);
void DrawTile8Keyed(int x, int y);
int BlitRawCopy(int x, int y, int width, int height, int arg);
int BlitRawCopyMirror(int x, int y, int width, int height, int arg);
int BlitRawKeyed(int x, int y, int width, int height, int arg);
void BlendPairs(std::uint32_t* pairs, std::uint32_t color, std::uint32_t count);
int BlitRawKeyedMirror(int x, int y, int width, int height, int arg);

}  // namespace swd3
