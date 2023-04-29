#include <alog/tools_internal.h>
#include <sys/stat.h>

#ifdef ALOG_OS_WINDOWS
    #define statX _stat64
#elif ALOG_OS_LINUX
    #define statX stat64
#elif ALOG_OS_MACOS
    #define statX stat
#endif // ALOG_OS_WINDOWS

namespace ALog {
namespace Internal {

std::optional<std::chrono::system_clock::time_point> getFileCreationTime(const char* fileName)
{
    struct statX fileInfo;

    if (auto error = statX(fileName, &fileInfo); error)
        return {};

    return std::chrono::system_clock::from_time_t(fileInfo.st_ctime);
}

std::optional<size_t> getFileSize(const char* fileName)
{
    struct statX fileInfo;

    if (auto error = statX(fileName, &fileInfo); error)
        return {};

    return fileInfo.st_size;
}

FilePathDetails analyzePath(const std::string& path)
{
    FilePathDetails result;
    std::optional<std::string::const_iterator> lastSeparator;
    std::optional<std::string::const_iterator> lastDot;

    for (auto it = path.cbegin(), end = path.cend(); it != end; ++it)
        if (*it == '/' || *it == '\\')
            lastSeparator = it + 1;

    for (auto it = lastSeparator.value_or(path.cbegin()),
         end = path.cend();
         it != end;
         ++it)
    {
        if (*it == '.')
            lastDot = it;
    }

    if (lastSeparator) result.path = std::string(path.cbegin(), *lastSeparator);
    result.baseName = std::string(lastSeparator.value_or(path.cbegin()), lastDot.value_or(path.cend()));
    if (lastDot) result.extension = std::string(*lastDot, path.cend());

    return result;
}

} // namespace Internal
} // namespace ALog
