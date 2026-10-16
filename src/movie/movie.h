#pragma once

#include <cstdint>

#include "swd3/bink.h"

namespace swd3 {

// Bink movie player (0x484 bytes; one instance at G.movie while a movie plays).
class Movie {
public:
    Movie();
    int Open(const char* name, int unused, int volume, int window);
    int Close();
    int Frame(void* dest, int pitch, int height, int x, int y);
    int Position();
    bool Waiting();
    int Width();
    int Height();

    bink::HBINK m_bink;            // +0x000
    std::int32_t unknown4;         // +0x004
    bink::BINKSUMMARY m_summary;   // +0x008
    char m_error[0x400];           // +0x084
};

static_assert(sizeof(Movie) == 0x484);

void MovieInit(void* surface, char directSound, int soundParam);

}  // namespace swd3
