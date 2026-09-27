/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/alog
 * Contact:  ihor-drachuk-libs@pm.me  */

#pragma once
#include <alog/sink.h>
#include <memory>
#include <optional>
#include <string>

#if __has_include(<version>)
#include <version>
#endif

#if defined(__cpp_lib_expected) && __cpp_lib_expected >= 202202L
#include <expected>
#endif

namespace ALog {
namespace Sinks {

class FileRotated : public ISink
{
public:
    FileRotated(const std::string& fileName,
                bool createPath = false,
                std::optional<size_t> maxFileSize /* bytes */ = {},
                std::optional<size_t> maxFileAge /* days */ = {},
                std::optional<size_t> maxFilesCount = {});  // throws
    ~FileRotated() override;

    // Non-throwing construction: nullptr on failure, the reason goes to `error` (if given).
    // For callers built without exceptions, where the throwing constructor would terminate.
    [[nodiscard]] static std::shared_ptr<FileRotated> tryCreate(const std::string& fileName,
                                                                bool createPath = false,
                                                                std::optional<size_t> maxFileSize = {},
                                                                std::optional<size_t> maxFileAge = {},
                                                                std::optional<size_t> maxFilesCount = {},
                                                                std::string* error = nullptr) noexcept;

#if defined(__cpp_lib_expected) && __cpp_lib_expected >= 202202L
    // Non-throwing construction for C++23 callers; inline, so it is available even when ALog itself is built as C++17.
    [[nodiscard]] static std::expected<std::shared_ptr<FileRotated>, std::string> create(const std::string& fileName,
                                                                                          bool createPath = false,
                                                                                          std::optional<size_t> maxFileSize = {},
                                                                                          std::optional<size_t> maxFileAge = {},
                                                                                          std::optional<size_t> maxFilesCount = {}) noexcept
    {
        std::string error;
        auto sink = tryCreate(fileName, createPath, maxFileSize, maxFileAge, maxFilesCount, &error);
        if (!sink)
            return std::unexpected(std::move(error));
        return sink;
    }
#endif

    void write(const Buffer& buffer, const Record& record) override;
    void flush() override;

private:
    struct FileContext;
    void rotate();        // throws
    void checkMaxCount(); // throws
    void checkMaxSize();  // throws
    void checkMaxAge();   // throws
    void correctFileNames(); // throws
    [[nodiscard]] FileContext createInitialContext() const;

private:
    ALOG_DECLARE_PIMPL
};

} // namespace Sinks
} // namespace ALog
