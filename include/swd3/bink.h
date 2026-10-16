#pragma once

#include <cstdint>

// RAD Bink (binkw32.dll) entry points used by swd32.exe, emulated with FFmpeg (src/platform/bink.cpp): the
// video is decoded frame by frame and converted for BinkCopyToBuffer, the audio is played on its own SDL3
// audio stream. Only the public head of the BINK structure is laid out like the SDK; swd32.exe reads Frames
// and FrameNum.
namespace bink {

using S32 = std::int32_t;
using U32 = std::uint32_t;

struct Player;

struct BINK {
    U32 Width;         // +0x00
    U32 Height;        // +0x04
    U32 Frames;        // +0x08
    U32 FrameNum;      // +0x0C 1-based current frame
    U32 LastFrameNum;  // +0x10
    U32 FrameRate;     // +0x14
    U32 FrameRateDiv;  // +0x18
    Player* player;    // emulation state
};
using HBINK = BINK*;

// The 0x7C byte summary filled by BinkGetSummary; swd32.exe uses Width and Height.
struct BINKSUMMARY {
    U32 Width;
    U32 Height;
    U32 TotalTime;
    U32 FileFrameRate;
    U32 FileFrameRateDiv;
    U32 FrameRate;
    U32 FrameRateDiv;
    U32 unused[24];
};
static_assert(sizeof(BINKSUMMARY) == 0x7C);

using SoundSystemOpen = S32 (*)(U32 param);

// Surface types of BinkDDSurfaceType / BinkCopyToBuffer.
constexpr U32 BINKSURFACE565 = 10;

S32 BinkOpenDirectSound(U32 param);
S32 BinkOpenMiles(U32 param);
S32 BinkSetSoundSystem(SoundSystemOpen open, U32 param);
S32 BinkDDSurfaceType(void* surface);
HBINK BinkOpen(const char* name, U32 flags);
const char* BinkGetError();
void BinkGetSummary(HBINK bink, BINKSUMMARY* summary);
void BinkSetVolume(HBINK bink, S32 volume);
void BinkClose(HBINK bink);
S32 BinkDoFrame(HBINK bink);
S32 BinkCopyToBuffer(HBINK bink, void* dest, S32 destPitch, U32 destHeight, U32 destX, U32 destY, U32 flags);
void BinkNextFrame(HBINK bink);
void BinkService(HBINK bink);
S32 BinkWait(HBINK bink);

}  // namespace bink
