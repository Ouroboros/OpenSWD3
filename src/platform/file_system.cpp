#include "platform/file_system.h"

#include <cctype>
#include <filesystem>
#include <system_error>

namespace swd3 {

namespace fs = std::filesystem;

namespace {

bool sameIgnoringCase(std::string_view left, std::string_view right) {
    if (left.size() != right.size()) {
        return false;
    }
    for (std::size_t i = 0; i < left.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(left[i])) != std::tolower(static_cast<unsigned char>(right[i]))) {
            return false;
        }
    }
    return true;
}

fs::path matchComponent(const fs::path& directory, const std::string& name) {
    std::error_code error;
    fs::path direct = directory / name;
    if (fs::exists(direct, error) || name == "." || name == "..") {
        return direct;
    }
    const fs::path searched = directory.empty() ? fs::path(".") : directory;
    for (const auto& entry : fs::directory_iterator(searched, error)) {
        const std::string entryName = entry.path().filename().string();
        if (sameIgnoringCase(entryName, name)) {
            return directory / entryName;
        }
    }
    return direct;
}

}  // namespace

std::string ResolvePath(std::string_view gamePath) {
    std::string path(gamePath);
    for (char& c : path) {
        if (c == '\\') {
            c = '/';
        }
    }
    std::error_code error;
    if (path.empty() || fs::exists(path, error)) {
        return path;
    }
    fs::path result = path.front() == '/' ? fs::path("/") : fs::path();
    std::size_t start = 0;
    while (start <= path.size()) {
        const std::size_t end = path.find('/', start);
        const std::string part = path.substr(start, end == std::string::npos ? std::string::npos : end - start);
        if (!part.empty()) {
            result = matchComponent(result, part);
        }
        if (end == std::string::npos) {
            break;
        }
        start = end + 1;
    }
    return result.generic_string();
}

}  // namespace swd3
