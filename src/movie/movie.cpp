#include "movie/movie.h"

#include "platform/message_box.h"

namespace swd3
{

// swd32.exe 0x00484550: the decoder plays the sound on its own output and copies RGB565 frames
void Movie::Init(bool, bool)
{
}

// swd32.exe 0x00484590
Movie::Movie() = default;

// swd32.exe 0x004845A0: a failed open keeps Bink's error text and a zero summary
void Movie::Open(const std::string& path, int volume)
{
    if (!this->decoder.Open(path))
    {
        this->error = this->decoder.Error();
    }

    this->width = this->decoder.Width();
    this->height = this->decoder.Height();

    if (volume < 0)
    {
        this->decoder.SetVolume(0);
        return;
    }

    this->decoder.SetVolume(volume > Movie::kMaxVolume ? Movie::kMaxVolume : volume);
}

// swd32.exe 0x00484620
bool Movie::Close()
{
    if (!this->decoder.IsOpen())
    {
        return false;
    }

    this->decoder.Close();
    return true;
}

// swd32.exe 0x00484650: shows the current frame at (x, y) and advances; the frame count, negated once the last frame
// is shown
int Movie::Frame(Surface& surface, int x, int y, ErrorReporter& reporter)
{
    this->decoder.DecodeFrame();

    if (!this->decoder.CopyTo(surface, x, y))
    {
        reporter.ShowError("fail", "BinkCopyToBuffer Fial");
    }

    const u32 frames = this->decoder.Frames();
    int result = -static_cast<int>(frames);

    if (frames > this->decoder.FrameNum())
    {
        result = static_cast<int>(frames);
        this->decoder.NextFrame();
    }

    return result;
}

// A movie is open (the original keeps one behind 0x0053D070 from 0x00484730 to 0x00484920).
bool Movie::IsOpen() const
{
    return this->decoder.IsOpen();
}

// swd32.exe 0x004846D0: the current frame, negated past the end; -1 without a movie
int Movie::Position() const
{
    if (!this->decoder.IsOpen())
    {
        return -1;
    }

    const u32 frame = this->decoder.FrameNum();

    if (frame > this->decoder.Frames())
    {
        return -static_cast<int>(frame);
    }

    return static_cast<int>(frame);
}

// swd32.exe 0x004846F0
bool Movie::Waiting() const
{
    return this->decoder.IsOpen() && this->decoder.Wait();
}

// swd32.exe 0x00484710
int Movie::Width() const
{
    return this->decoder.IsOpen() ? this->width : 0;
}

// swd32.exe 0x00484720
int Movie::Height() const
{
    return this->decoder.IsOpen() ? this->height : 0;
}

const std::string& Movie::Error() const
{
    return this->error;
}

}  // namespace swd3
