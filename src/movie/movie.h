#pragma once

#include <string>

#include "core/types.h"
#include "platform/media_decoder.h"

namespace swd3
{

class ErrorReporter;
class Surface;

// A Bink movie of the game (0x484 bytes in the original: the Bink handle, its 0x7C-byte summary and the error text).
class Movie
{
public:
    static constexpr int MaxVolume = 0x8000;

    // Bink's sound output (DirectSound or the Miles digital driver) and the copy format of the back buffer.
    static void Init(bool directSound, bool digitalDriver);

    Movie();

    void Open(const std::u8string& path, int volume);
    bool Close();
    int Frame(Surface& surface, int x, int y, ErrorReporter& reporter);
    int Position() const;
    bool Waiting() const;
    bool IsOpen() const;
    int Width() const;
    int Height() const;
    const std::string& Error() const;

private:
    MediaDecoder decoder;
    int width = 0;
    int height = 0;
    std::string error;
};

}  // namespace swd3
