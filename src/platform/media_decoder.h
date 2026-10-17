#pragma once

#include <memory>
#include <string>

#include "core/types.h"

namespace swd3
{

class Surface;

// A Bink movie (what the game reaches through binkw32.dll), decoded with FFmpeg: the video frame by frame as RGB565,
// the audio on its own output. Frames are numbered from 1 like Bink's FrameNum.
class MediaDecoder
{
public:
    MediaDecoder();
    ~MediaDecoder();
    MediaDecoder(const MediaDecoder&) = delete;
    MediaDecoder& operator=(const MediaDecoder&) = delete;

    bool Open(const std::u8string& path);
    void Close();
    bool IsOpen() const;
    // Why the last Open failed (BinkGetError).
    const std::string& Error() const;

    int Width() const;
    int Height() const;
    u32 Frames() const;
    u32 FrameNum() const;
    // 0..0x8000 (Bink's volume scale).
    void SetVolume(int volume);
    // Decodes the current frame (once; BinkDoFrame).
    void DecodeFrame();
    // The decoded frame at (x, y) of `surface`, clipped to it (BinkCopyToBuffer).
    bool CopyTo(Surface& surface, int x, int y) const;
    // Advances to the next frame (BinkNextFrame).
    void NextFrame();
    // Whether it is too early to show the current frame (BinkWait).
    bool Wait() const;

private:
    struct State;

    std::unique_ptr<State> state;
    std::string error;
};

}  // namespace swd3
