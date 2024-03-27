# Disable warning
# "Compatibility with CMake < 3.5 will be removed from a future version of CMake."
# It's produced by GTest and likely Google benchmark.
set(CMAKE_WARN_DEPRECATED OFF CACHE BOOL "" FORCE)

# Fix warning for FetchContent_Declare regarding DOWNLOAD_EXTRACT_TIMESTAMP
if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.24")
    cmake_policy(SET CMP0135 NEW)
endif()

set(ALOG_NEED_GTEST OFF)
set(ALOG_NEED_BENCHMARK OFF)

if (NOT ALOG_ENABLE_TESTS AND NOT ALOG_ENABLE_BENCHMARK)
    return()
endif()

include(FetchContent)

if (ALOG_ENABLE_TESTS)
    set(ALOG_NEED_GTEST ON)
endif()

if (ALOG_ENABLE_BENCHMARK)
    set(ALOG_NEED_GTEST ON)
    set(ALOG_NEED_BENCHMARK ON)
endif()

if (NOT ${ALOG_GTEST_SEARCH_MODE} STREQUAL "Skip")
    if (${ALOG_GTEST_SEARCH_MODE} STREQUAL "Auto")
        set(ALOG__INTERNAL_GTEST_SEARCH_MODE QUIET)
    else()
        set(ALOG__INTERNAL_GTEST_SEARCH_MODE REQUIRED)
    endif()
endif()

if (ALOG_NEED_GTEST AND NOT TARGET gtest)
    if (NOT ${ALOG_GTEST_SEARCH_MODE} STREQUAL "Skip")
        find_package(GTest ${ALOG__INTERNAL_GTEST_SEARCH_MODE})
    endif()

    if (GTest_FOUND)
        message("GTest found!")
    else()
        message("GTest NOT found! Adding external source...")

        set(INSTALL_GTEST OFF CACHE BOOL "Do not install gtest" FORCE)
        set(INSTALL_GMOCK OFF CACHE BOOL "Do not install gmock" FORCE)
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

        FetchContent_Declare(
            googletest
            URL https://github.com/google/googletest/archive/refs/tags/release-1.11.0.tar.gz
        )

        FetchContent_MakeAvailable(googletest)
    endif()
endif()

if (ALOG_NEED_BENCHMARK AND NOT TARGET benchmark)
    if (NOT ${ALOG_GTEST_SEARCH_MODE} STREQUAL "Skip")
        find_package(benchmark ${ALOG__INTERNAL_GTEST_SEARCH_MODE})
    endif()

    if (benchmark_FOUND)
        message("Google benchmark found!")
    else()
        message("Google benchmark NOT found! Adding external source...")

        set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
        set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
        set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)

        FetchContent_Declare(
            google-benchmark
            URL https://github.com/google/benchmark/archive/refs/tags/v1.8.3.tar.gz
        )

        FetchContent_MakeAvailable(google-benchmark)
    endif()
endif()

enable_testing()

if (ALOG_ENABLE_TESTS)
    add_subdirectory(test)
    add_subdirectory(test2)
endif()

if (ALOG_ENABLE_BENCHMARK)
    add_subdirectory(benchmark)
endif()
