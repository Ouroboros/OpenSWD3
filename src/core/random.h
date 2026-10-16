#pragma once

#include <cstdint>

// R250 shift-register generator seeded by a 0x015A4E35 linear congruential generator.
namespace swd3 {

void R250Seed(std::uint32_t seed);
std::uint32_t R250Next();
std::uint32_t R250Below(std::uint32_t limit);
std::uint32_t LcgNext();
std::uint32_t LcgSeed(std::uint32_t seed);

}  // namespace swd3
