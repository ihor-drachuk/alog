#pragma once

#if __has_include(<filesystem>)
    #include <filesystem>

    static bool fsExists(const std::filesystem::directory_entry& x) {
        return x.exists();
    }

    static bool fsIsDirectory(const std::filesystem::directory_entry& x) {
        return x.is_directory();
    }

    static uintmax_t fsSize(const std::filesystem::directory_entry& x) {
        return x.file_size();
    }
#else
    #include <experimental/filesystem>

    namespace std {
        using namespace experimental;
    } // namespace std

    static bool fsExists(const std::filesystem::directory_entry& x) {
        return std::filesystem::exists(x);
    }

    static bool fsIsDirectory(const std::filesystem::directory_entry& x) {
        return std::filesystem::is_directory(x);
    }

    static uintmax_t fsSize(const std::filesystem::directory_entry& x) {
        return std::filesystem::file_size(x);
    }
#endif // __has_include(<filesystem>)
