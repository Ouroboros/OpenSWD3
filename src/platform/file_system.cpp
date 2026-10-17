#include "platform/file_system.h"

#include <array>

namespace swd3
{

namespace
{

constexpr std::size_t kReadChunk = 0x10000;

}  // namespace

bool ReadSharedFile(const std::string& path, std::vector<u8>& bytes)
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

}  // namespace swd3
