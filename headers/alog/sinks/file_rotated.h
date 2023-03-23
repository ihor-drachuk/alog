#pragma once
#include <alog/sink.h>
#include <optional>
#include <string>

namespace ALog {
namespace Sinks {

class FileRotated : public ISink
{
public:
    FileRotated(const std::string& fileName,
                std::optional<size_t> maxFileSize,
                std::optional<size_t> maxFileAge, // days
                std::optional<size_t> maxFilesCount); // throws
    ~FileRotated() override;

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
