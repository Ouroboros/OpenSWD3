#include "movie/movie.h"

#include <cstring>

#include "data/globals.h"
#include "swd3/w32.h"

namespace swd3 {

// swd32.exe 0x00484550: selects the Bink sound output and the surface type for Movie::Frame. The return value is
// unused by callers.
void MovieInit(void* surface, char directSound, int soundParam) {
    if (directSound != 0) {
        bink::BinkSetSoundSystem(&bink::BinkOpenDirectSound, static_cast<bink::U32>(soundParam));
    } else {
        bink::BinkSetSoundSystem(&bink::BinkOpenMiles, static_cast<bink::U32>(soundParam));
    }
    G.movieSurfaceType = bink::BinkDDSurfaceType(surface);
}

// swd32.exe 0x00484590
Movie::Movie() {
    m_bink = nullptr;
    unknown4 = 0;
}

// swd32.exe 0x004845A0: opens `name` (the Bink error text kept on failure) and sets the volume (0..0x8000,
// negative mutes). Always returns 0.
int Movie::Open(const char* name, int unused, int volume, int window) {
    static_cast<void>(unused);
    static_cast<void>(window);
    if (name == nullptr) {
        return 0;
    }
    m_bink = bink::BinkOpen(name, 0);
    if (m_bink == nullptr) {
        w32::lstrcpyA(m_error, bink::BinkGetError());
    }
    std::memset(&m_summary, 0, sizeof(m_summary));
    bink::BinkGetSummary(m_bink, &m_summary);
    if (volume < 0) {
        bink::BinkSetVolume(m_bink, 0);
        return 0;
    }
    if (volume > 0x8000) {
        volume = 0x8000;
    }
    bink::BinkSetVolume(m_bink, volume);
    return 0;
}

// swd32.exe 0x00484620
int Movie::Close() {
    if (m_bink == nullptr) {
        return 0;
    }
    bink::BinkClose(m_bink);
    m_bink = nullptr;
    return 1;
}

// swd32.exe 0x00484650: decodes and copies the current frame, then advances; returns the frame count, negated
// once the last frame was shown
int Movie::Frame(void* dest, int pitch, int height, int x, int y) {
    bink::BinkDoFrame(m_bink);
    if (bink::BinkCopyToBuffer(m_bink, dest, pitch, static_cast<bink::U32>(height), static_cast<bink::U32>(x),
                               static_cast<bink::U32>(y), static_cast<bink::U32>(G.movieSurfaceType)) == 0) {
        w32::MessageBoxA(G.mainWindow, G.aBinkcopytobuff,
                         G.aFail, 0);
    }
    const bink::U32 frames = m_bink->Frames;
    int result = -static_cast<int>(frames);
    if (frames > m_bink->FrameNum) {
        result = static_cast<int>(frames);
        bink::BinkNextFrame(m_bink);
    }
    bink::BinkService(m_bink);
    return result;
}

// swd32.exe 0x004846D0: current frame, negated past the end; -1 without a movie
int Movie::Position() {
    if (m_bink == nullptr) {
        return -1;
    }
    const bink::U32 frame = m_bink->FrameNum;
    if (frame > m_bink->Frames) {
        return -static_cast<int>(frame);
    }
    return static_cast<int>(frame);
}

// swd32.exe 0x004846F0
bool Movie::Waiting() { return m_bink != nullptr && bink::BinkWait(m_bink) != 0; }

// swd32.exe 0x00484710
int Movie::Width() { return m_bink != nullptr ? static_cast<int>(m_summary.Width) : 0; }

// swd32.exe 0x00484720
int Movie::Height() { return m_bink != nullptr ? static_cast<int>(m_summary.Height) : 0; }

}  // namespace swd3
