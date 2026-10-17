#include "golden_check.h"

#include <array>
#include <cinttypes>
#include <cstdio>
#include <fstream>

namespace swd3test
{

namespace
{

constexpr int kShownMismatches = 256;
constexpr std::size_t kHashText = 17;  // sixteen hex digits and the NUL

}  // namespace

std::map<std::string, std::string> LoadGolden(const std::string& path)
{
    std::map<std::string, std::string> golden;
    std::ifstream file(path);
    std::string line;

    while (std::getline(file, line))
    {
        const std::size_t space = line.rfind(' ');

        if (space != std::string::npos)
        {
            golden[line.substr(0, space)] = line.substr(space + 1);
        }
    }

    return golden;
}

GoldenCheck::GoldenCheck(const std::map<std::string, std::string>& golden)
    : golden(golden)
{
}

void GoldenCheck::operator()(const std::string& name, swd3::u64 hash)
{
    std::array<char, kHashText> text = {};
    std::snprintf(text.data(), text.size(), "%016" PRIx64, hash);
    const auto found = this->golden.find(name);
    ++this->compared;

    if (found == this->golden.end() || found->second != text.data())
    {
        if (++this->mismatches <= kShownMismatches)
        {
            std::printf(
                "  mismatch: %s (%s, got %s)\n",
                name.c_str(),
                found == this->golden.end() ? "missing" : "hash",
                text.data()
            );
        }
    }
}

}  // namespace swd3test
