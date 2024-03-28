/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/alog
 * Contact:  ihor-drachuk-libs@pm.me  */

#pragma once

#if __has_include(<filesystem>)
    #include <filesystem>

    inline bool fsExists(const std::filesystem::directory_entry& x) {
        return x.exists();
    }

    inline bool fsIsDirectory(const std::filesystem::directory_entry& x) {
        return x.is_directory();
    }

    inline uintmax_t fsSize(const std::filesystem::directory_entry& x) {
        return x.file_size();
    }
#else
    #include <experimental/filesystem>

    namespace std {
        using namespace experimental;
    } // namespace std

    inline bool fsExists(const std::filesystem::directory_entry& x) {
        return std::filesystem::exists(x);
    }

    inline bool fsIsDirectory(const std::filesystem::directory_entry& x) {
        return std::filesystem::is_directory(x);
    }

    inline uintmax_t fsSize(const std::filesystem::directory_entry& x) {
        return std::filesystem::file_size(x);
    }
#endif // __has_include(<filesystem>)
