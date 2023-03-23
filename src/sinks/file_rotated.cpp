#include <alog/sinks/file_rotated.h>

#include <alog/sinks/file.h>
#include <alog/tools_internal.h>

#include <filesystem>
#include <stdexcept>
#include <regex>
#include <unordered_set>

namespace {
using Clock = std::chrono::system_clock;
using Time = Clock::time_point;
using Days = std::chrono::duration<int, std::ratio<60 * 60 * 24>>;
} // namespace

namespace ALog {
namespace Sinks {

struct FileRotated::FileContext
{
    std::filesystem::path path;
    std::optional<size_t> currentRotNo; // empty for current file
    size_t currentSize {};
    Time creationTime;

    bool operator< (const FileContext& rhs) const {
        if (currentRotNo && rhs.currentRotNo) {
            if (currentRotNo != rhs.currentRotNo) {
                return currentRotNo < rhs.currentRotNo;
            } else {
                // Fallthrough
            }

        } else if (!currentRotNo && !rhs.currentRotNo) {
            // Fallthrough

        } else {
            return !currentRotNo.has_value(); // Empty (i.e. "current") is less than "non-current"
        }

        return (creationTime < rhs.creationTime);
    }
};

struct FileRotated::impl_t
{
    std::string filePath;
    Internal::FilePathDetails filePathDetails;
    std::optional<size_t> maxFileSize;
    std::optional<Days>   maxFileAge;
    std::optional<size_t> maxFilesCount;

    std::vector<FileContext> files;

    std::unique_ptr<ALog::Sinks::File> logFile;
};

FileRotated::FileRotated(const std::string& fileName,
                         std::optional<size_t> maxFileSize,
                         std::optional<size_t> maxFileAge,
                         std::optional<size_t> maxFilesCount)
{
    createImpl();
    impl().filePath = fileName;
    impl().filePathDetails = Internal::analyzePath(fileName);
    impl().maxFileSize = maxFileSize;
    if (maxFileAge)
        impl().maxFileAge = Days(*maxFileAge);
    impl().maxFilesCount = maxFilesCount;

    if (impl().maxFileSize.value_or(1) < 1)
        throw std::runtime_error("Invalid 'maxFileSize' provided!");

    if (impl().maxFileAge.value_or(Days(1)) < Days(1))
        throw std::runtime_error("Invalid 'maxFileAge' provided!");

    if (impl().maxFilesCount.value_or(1) < 1)
        throw std::runtime_error("Invalid 'maxFilesCount' provided!");

    if (!std::filesystem::directory_entry(impl().filePathDetails.path).exists())
        throw std::runtime_error("No such directory!");

    // Find all matching logs
    const auto logsFileNameBase = impl().filePathDetails.baseName + impl().filePathDetails.extension;
    const auto fileNameEscaped = std::regex_replace(logsFileNameBase, std::regex(R"([-[\]{}()*+?.,\^$|#\s])"), R"(\$&)");
    std::regex logFileRegex ("^" + fileNameEscaped + R"((\.\d+)?$)");

    for (const auto& item : std::filesystem::directory_iterator(impl().filePathDetails.path)) {
        if (item.is_directory()) continue;

        const auto itemFn = item.path().filename().u8string();
        std::smatch itemFnMatch;
        if (!std::regex_search(itemFn, itemFnMatch, logFileRegex)) continue;
        assert(itemFnMatch.size() == 2);

        FileContext context;
        context.path = item;
        if (itemFnMatch[1].matched)
            context.currentRotNo = std::stoul(itemFnMatch[1].str().substr(1)); // throws
        context.currentSize = item.file_size();
        context.creationTime = Internal::getFileCreationTime(item.path().u8string().c_str()).value(); // throws

        impl().files.push_back(std::move(context));
    }

    // Sort files list
    std::sort(impl().files.begin(), impl().files.end());

    // Make sure "current" file is at most one
#ifndef NDEBUG // Is debug
    {
        const auto currentFiles = std::count_if(impl().files.cbegin(), impl().files.cend(),
                                                [](const FileContext& ctx){ return !ctx.currentRotNo.has_value(); });
        assert(currentFiles <= 1 && "Wrong regexp matched several 'current' files!");
    }
#endif // !NDEBUG (Is debug)

    // Make sure 'current' file exists
    if (impl().files.empty() || impl().files.front().currentRotNo.has_value())
        impl().files.insert(impl().files.cbegin(), createInitialContext());

    assert(impl().files.size() > 0);                        // There is least one (current) log file
    assert(!impl().files.front().currentRotNo.has_value()); // Make sure first is current

    // `currentRotNo` could be wrong if files was renamed manually
    //   - service.log.3, service.log.003
    //   - service.log.0  or  service.log.1
    //
    // But gaps are treated as ok (service.log.2, service.log.10, service.log.25)
    {
        std::unordered_set<size_t> numbers;
        bool needNumbersCorrection = false;
        for (const auto& x : impl().files)
            if (x.currentRotNo && !numbers.insert(*x.currentRotNo).second) {
                needNumbersCorrection = true;
                break;
            }

        if (needNumbersCorrection || numbers.count(0) || numbers.count(1))
            correctFileNames(); // throws
    }

    checkMaxCount(); // throws
    checkMaxSize(); // throws
    checkMaxAge(); // throws

    // Create/Open current file
    impl().logFile = std::make_unique<ALog::Sinks::File>(impl().files.front().path.u8string().c_str()); // throws
}

FileRotated::~FileRotated()
{
}

void FileRotated::write(const Buffer& buffer, const Record& record)
{
    try {
        // Check max size before writing
        if (impl().maxFileSize && impl().logFile->expectedNewSize(buffer) > *impl().maxFileSize) {
            rotate(); // throws
            assert(impl().logFile->expectedNewSize(buffer) <= *impl().maxFileSize);
        }

        // Also check age
        checkMaxAge(); // throws

        // Write
        impl().logFile->write(buffer, record); // error not handled
        impl().files.front().currentSize = impl().logFile->getSize();

    } catch (...) {
        // TODO: Implement error handling when it's support will be integrated to Logger
        //       Also pay attention to `impl().logFile->write`.
        assert(false && "FileRotated::write failed!");
    }
}

void FileRotated::flush()
{
    impl().logFile->flush();
}

void FileRotated::rotate()
{
    // Close file (if opened)
    const bool wasOpened = static_cast<bool>(impl().logFile);
    impl().logFile.reset();

    // Rotate
    if (impl().maxFilesCount && impl().files.size() >= *impl().maxFilesCount) {
        assert(impl().files.size() == *impl().maxFilesCount);
        std::filesystem::remove(impl().files.back().path); // throws
        impl().files.pop_back();
    }

    for (size_t i = impl().files.size() - 1; i >= 1; i--) {
        auto& fileCtx = impl().files[i];

        assert(fileCtx.currentRotNo);
        const auto newRotNo = *fileCtx.currentRotNo + 1;
        const auto newPath = std::filesystem::path(fileCtx.path).replace_extension(std::to_string(newRotNo));

        std::filesystem::rename(fileCtx.path, newPath); // throws

        fileCtx.currentRotNo = newRotNo;
        fileCtx.path = newPath;
    }

    const auto newRotNo = 2;
    const auto newPath = (std::filesystem::path(impl().files.front().path) += ".2");
    std::filesystem::rename(impl().files.front().path, newPath); // throws
    impl().files.front().currentRotNo = newRotNo;
    impl().files.front().path = newPath;

    impl().files.insert(impl().files.cbegin(), createInitialContext());
    assert(!std::filesystem::exists(impl().files.front().path) && "Unexpected result: 'current' log file remains after rotation!");

    // Open file (if was opened)
    if (wasOpened)
        impl().logFile = std::make_unique<ALog::Sinks::File>(impl().files.front().path.u8string().c_str()); // throws
}

void FileRotated::checkMaxCount()
{
    if (impl().maxFilesCount) {
        while (impl().files.size() > *impl().maxFilesCount) {
            std::filesystem::remove(impl().files.back().path);  // throws
            impl().files.pop_back();
        }
    }
}

void FileRotated::checkMaxSize()
{
    if (impl().maxFileSize && impl().files.front().currentSize > *impl().maxFileSize)
        rotate(); // throws
}

void FileRotated::checkMaxAge()
{
    if (impl().maxFileAge  && Clock::now() > impl().files.front().creationTime + *impl().maxFileAge)
        rotate(); // throws
}

void FileRotated::correctFileNames()
{
    const auto firstFreeRotNo = std::max_element(impl().files.cbegin(), impl().files.cend())->currentRotNo.value_or(0) + 1;

    std::vector<std::pair<size_t, std::filesystem::path>> newNames;
    newNames.push_back({}); // Align vectors

    for (size_t i = 1; i < impl().files.size(); i++) {
        auto& fileCtx = impl().files[i];
        assert(fileCtx.currentRotNo.has_value() && "'Current' file can be at most one. Others should be numbered");

        const auto newRotNo = i + 1; // Min is '2'
        const auto tempRotNo = firstFreeRotNo + i; // Min is '+1'

        const auto newPath = std::filesystem::path(fileCtx.path).replace_extension(std::to_string(newRotNo));
        const auto tempPath = std::filesystem::path(fileCtx.path).replace_extension(std::to_string(tempRotNo));

        std::filesystem::rename(fileCtx.path, tempPath); // throws
        fileCtx.currentRotNo = tempRotNo;
        fileCtx.path = tempPath;

        newNames.push_back(std::make_pair(newRotNo, newPath));
    }

    for (size_t i = 1; i < impl().files.size(); i++) {
        auto& fileCtx = impl().files[i];
        const auto& newName = newNames.at(i);

        std::filesystem::rename(fileCtx.path, newName.second); // throws
        fileCtx.currentRotNo = newName.first;
        fileCtx.path = newName.second;
    }
}

FileRotated::FileContext FileRotated::createInitialContext() const
{
    FileContext context;
    context.path = impl().filePath;
    //context.currentRotNo leave empty
    context.currentSize = 0;
    context.creationTime = Clock::now();

    return context;
}

} // namespace Sinks
} // namespace ALog
