/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/alog
 * Contact:  ihor-drachuk-libs@pm.me  */

#include <alog/tools_internal.h>
#include <sys/stat.h>
#include <cstdlib>
#include <array>

#ifdef ALOG_OS_WINDOWS
    #include <Windows.h>
    #include <io.h>
    #define statX _stat64
    #define isatty _isatty
    #define fileno _fileno
#elif ALOG_OS_LINUX
    #include <unistd.h>
    #define statX stat64
#elif ALOG_OS_MACOS
    #include <unistd.h>
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

    return static_cast<size_t>(fileInfo.st_size);
}

FilePathDetails analyzePath(const std::string& path)
{
    FilePathDetails result;
    std::optional<std::string::const_iterator> optLastSeparatorIt;
    std::optional<std::string::const_iterator> optLastDotIt;

    for (auto it = path.cbegin(), end = path.cend(); it != end; ++it)
        if (*it == '/' || *it == '\\')
            optLastSeparatorIt = it + 1;

    for (auto it = optLastSeparatorIt.value_or(path.cbegin()),
         end = path.cend();
         it != end;
         ++it)
    {
        if (*it == '.')
            optLastDotIt = it;
    }

    if (optLastSeparatorIt) result.path = std::string(path.cbegin(), *optLastSeparatorIt);
    result.baseName = std::string(optLastSeparatorIt.value_or(path.cbegin()), optLastDotIt.value_or(path.cend()));
    if (optLastDotIt) result.extension = std::string(*optLastDotIt, path.cend());

    return result;
}

bool enableColoredTerminal(FILE* stream)
{
    const auto qtCreator = std::getenv("QT_CREATOR_RUNNING") || std::getenv("QT_CREATOR");

    const bool isTerminal = qtCreator || (isatty(fileno(stream)) != 0);
    if (!isTerminal)
       return false;

#ifdef ALOG_OS_WINDOWS
    if (!qtCreator) {
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        if (hOut == INVALID_HANDLE_VALUE)
            return false;

        DWORD dwMode = 0;
        if (!GetConsoleMode(hOut, &dwMode))
            return false;

        DWORD newMode = dwMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        if (!SetConsoleMode(hOut, newMode))
            return false;
    }

    return true;

#elif ALOG_OS_LINUX
    return true;

#elif ALOG_OS_MACOS
    return true;
#endif // ALOG_OS_WINDOWS, ALOG_OS_LINUX, ALOG_OS_MACOS
}

const std::string& getSeverityColorCode(Severity severity)
{
    const static std::array<std::string, Severity::COUNT> colors {
        "\033[38;5;244m",
        "\033[38;5;246m",
        "\033[38;5;39m",
        "\033[38;5;178m",
        "\033[38;5;196m",
        "\033[1;38;5;199m"
    };

    return colors[severity];
}

const std::string& getResetColorCode()
{
    const static std::string code = "\033[0m";
    return code;
}

} // namespace Internal
} // namespace ALog
