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

if (ALOG_NEED_GTEST AND NOT TARGET gtest)
    find_package(GTest QUIET)

    if (GTest_FOUND)
        message("GTest found!")
    else()
        message("GTest NOT found! Going to download...")

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
    find_package(benchmark QUIET)
    
    if (benchmark_FOUND)
        message("Google benchmark found!")
    else()
        message("Google benchmark NOT found! Going to download...")

        set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
        set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
        set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)

        FetchContent_Declare(
            google-benchmark
            URL https://github.com/google/benchmark/archive/refs/tags/v1.5.3.tar.gz
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
