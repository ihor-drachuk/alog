/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/alog
 * Contact:  ihor-drachuk-libs@pm.me  */

#pragma once
#include <optional>
#include <chrono>
#include <string>
#include <tuple>

namespace ALog {
namespace Internal {

struct FilePathDetails
{
    std::string path;
    std::string baseName;
    std::string extension;

    auto tie() const { return std::tie(path, baseName, extension); }
    bool operator== (const FilePathDetails& rhs) const { return tie() == rhs.tie(); }
};

std::optional<std::chrono::system_clock::time_point> getFileCreationTime(const char* fileName);
std::optional<size_t> getFileSize(const char* fileName);
FilePathDetails analyzePath(const std::string& path);

} // namespace Internal
} // namespace ALog
