#include "platform/path_posix.h"

#include <dirent.h>
#include <strings.h>
#include <sys/stat.h>

#include <vector>

namespace w32::posix {
namespace {

bool exists(const std::string& path) {
    struct stat st {};
    return ::stat(path.c_str(), &st) == 0;
}

std::string join(const std::string& dir, const std::string& name) {
    if (dir.empty()) {
        return name;
    }
    if (dir == "/") {
        return "/" + name;
    }
    return dir + "/" + name;
}

std::string matchComponent(const std::string& dir, const std::string& name) {
    std::string direct = join(dir, name);
    if (exists(direct) || name == "." || name == "..") {
        return direct;
    }
    DIR* handle = ::opendir(dir.empty() ? "." : dir.c_str());
    if (!handle) {
        return direct;
    }
    std::string found = direct;
    while (dirent* entry = ::readdir(handle)) {
        if (::strcasecmp(entry->d_name, name.c_str()) == 0) {
            found = join(dir, entry->d_name);
            break;
        }
    }
    ::closedir(handle);
    return found;
}

}  // namespace

std::string resolvePath(const char* windowsPath) {
    std::string path = windowsPath ? windowsPath : "";
    for (char& c : path) {
        if (c == '\\') {
            c = '/';
        }
    }
    if (path.empty() || exists(path)) {
        return path;
    }
    std::vector<std::string> parts;
    std::string current;
    for (char c : path) {
        if (c == '/') {
            if (!current.empty()) {
                parts.push_back(current);
            }
            current.clear();
        } else {
            current.push_back(c);
        }
    }
    if (!current.empty()) {
        parts.push_back(current);
    }
    std::string result = path[0] == '/' ? "/" : "";
    for (const std::string& part : parts) {
        result = matchComponent(result, part);
    }
    if (path.back() == '/' && !result.empty() && result.back() != '/') {
        result.push_back('/');
    }
    return result;
}

}  // namespace w32::posix
