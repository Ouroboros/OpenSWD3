#include "platform/file_system.h"

#include <SDL3/SDL.h>

#include <array>

#include "core/format.h"

namespace swd3
{

namespace
{

constexpr std::size_t kReadChunk = 0x10000;
constexpr std::u8string_view kAssetDirectory = u8"asset/";

}  // namespace

bool ReadSharedFile(const std::u8string& path, std::vector<u8>& bytes)
{
    std::FILE* file = OpenSharedFile(path, "rb");

    if (file == nullptr)
    {
        return false;
    }

    bytes.clear();
    std::array<u8, kReadChunk> chunk{};
    std::size_t read = 0;

    while ((read = std::fread(chunk.data(), 1, chunk.size(), file)) != 0)
    {
        bytes.insert(bytes.end(), chunk.begin(), chunk.begin() + static_cast<std::ptrdiff_t>(read));
    }

    const bool failed = std::ferror(file) != 0;
    std::fclose(file);
    return !failed;
}

std::u8string AssetDirectory()
{
    std::u8string path;

    if (const auto* base = SDL_GetBasePath(); base != nullptr)
    {
        path = Utf8Bytes(base);
    }

    return path + std::u8string(kAssetDirectory);
}

}  // namespace swd3
