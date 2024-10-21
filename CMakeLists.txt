cmake_minimum_required(VERSION 3.16)
PROJECT(alog LANGUAGES CXX)

option(ALOG_ENABLE_TESTS             "ALog: Enable tests" OFF)
option(ALOG_ENABLE_BENCHMARK         "ALog: Enable benchmark" OFF)
option(ALOG_ENABLE_DEF_SEPARATORS    "ALog: Enable space-separators by default" OFF)
option(ALOG_ENABLE_DEF_AUTO_QUOTES   "ALog: Enable auto quotes" ON)
option(ALOG_ENABLE_DEBUG             "ALog: Enable additional debug checks" OFF)

set(ALOG_CXX_STANDARD "17" CACHE STRING "ALog: Set C++ standard")
set_property(CACHE ALOG_CXX_STANDARD PROPERTY STRINGS "17" "20" "23")

set(ALOG_QT_SEARCH_MODE "Auto" CACHE STRING "ALog: Set Qt search mode")
set_property(CACHE ALOG_QT_SEARCH_MODE PROPERTY STRINGS "Auto" "Force" "Skip")

set(ALOG_GTEST_SEARCH_MODE "Auto" CACHE STRING "ALog: Set GTest search mode")
set_property(CACHE ALOG_GTEST_SEARCH_MODE PROPERTY STRINGS "Auto" "Force" "Skip")

add_subdirectory(3rd-party)

FILE(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp src/*.h headers/*.h)

add_library(alog STATIC ${SOURCES})
target_include_directories(alog PUBLIC headers)
target_link_libraries(alog
    PUBLIC jeaiii_to_text
)

if (ALOG_CXX_STANDARD STREQUAL "17")
    target_compile_features(alog PUBLIC cxx_std_17)
    target_compile_definitions(alog PUBLIC ALOG_CXX17)
    message(STATUS "ALog: Using C++17")

elseif (ALOG_CXX_STANDARD STREQUAL "20")
    target_compile_features(alog PUBLIC cxx_std_20)
    target_compile_definitions(alog PUBLIC ALOG_CXX17)
    target_compile_definitions(alog PUBLIC ALOG_CXX20)
    message(STATUS "ALog: Using C++20")

elseif (ALOG_CXX_STANDARD STREQUAL "23")
    target_compile_features(alog PUBLIC cxx_std_23)
    target_compile_definitions(alog PUBLIC ALOG_CXX17)
    target_compile_definitions(alog PUBLIC ALOG_CXX20)
    target_compile_definitions(alog PUBLIC ALOG_CXX23)
    message(STATUS "ALog: Using C++23")

else()
    message(FATAL_ERROR "Unsupported C++ standard: ${ALOG_CXX_STANDARD}")
endif()

if (NOT ${ALOG_QT_SEARCH_MODE} STREQUAL "Skip")

    if (${ALOG_QT_SEARCH_MODE} STREQUAL "Auto")
        set(ALOG__INTERNAL_QT_SEARCH_MODE QUIET)
    else()
        set(ALOG__INTERNAL_QT_SEARCH_MODE REQUIRED)
    endif()

    find_package(Qt6 COMPONENTS Core Gui QUIET)
    if (NOT Qt6_FOUND)
        find_package(Qt5 5.15 COMPONENTS Core Gui ${ALOG__INTERNAL_QT_SEARCH_MODE})
    endif()
endif()

if(TARGET p7)
    target_link_libraries(alog PRIVATE p7)
    target_compile_definitions(alog PUBLIC ALOG_HAS_P7_LIBRARY)
endif()

if(Qt5_FOUND OR Qt6_FOUND)
    target_link_libraries(alog PUBLIC Qt${QT_VERSION_MAJOR}::Core)

    set(ALOG_HAS_QT_LIBRARY TRUE)
    target_compile_definitions(alog PUBLIC ALOG_HAS_QT_LIBRARY)
endif()

if(MSVC)
    target_link_options(alog PRIVATE "/ignore:4221")
    set_target_properties(alog PROPERTIES STATIC_LIBRARY_OPTIONS "/ignore:4221")
endif()

target_compile_options(alog PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/WX>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Werror>
)

target_compile_definitions(alog PRIVATE NOMINMAX)

if(ALOG_ENABLE_DEF_SEPARATORS)
    target_compile_definitions(alog PUBLIC ALOG_ENABLE_DEF_SEPARATORS)
endif()

if(ALOG_ENABLE_DEF_AUTO_QUOTES)
    target_compile_definitions(alog PUBLIC ALOG_ENABLE_DEF_AUTO_QUOTES)
endif()

if(ALOG_ENABLE_DEBUG)
    target_compile_definitions(alog PUBLIC ALOG_ENABLE_DEBUG)
endif()


# Detect OS
if (WIN32)
    set(ALOG_OS_WINDOWS TRUE)
    target_compile_definitions(alog PUBLIC ALOG_OS_WINDOWS=1)

elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(ALOG_OS_MACOS TRUE)
    target_compile_definitions(alog PUBLIC ALOG_OS_MACOS=1)
elseif(UNIX)
    set(ALOG_OS_LINUX TRUE)
    target_compile_definitions(alog PUBLIC ALOG_OS_LINUX=1)
    target_link_libraries(alog PRIVATE pthread -lstdc++fs)
endif()

# Detect compiler
if (MSVC)
    set(ALOG_COMPILER_MSVC TRUE)
    target_compile_definitions(alog PUBLIC ALOG_COMPILER_MSVC=1)

elseif (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    set(ALOG_COMPILER_GCC TRUE)
    target_compile_definitions(alog PUBLIC ALOG_COMPILER_GCC=1)

elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(ALOG_COMPILER_CLANG TRUE)
    target_compile_definitions(alog PUBLIC ALOG_COMPILER_CLANG=1)

    if (CMAKE_CXX_COMPILER_ID MATCHES "Apple")
        set(ALOG_COMPILER_APPLE_CLANG TRUE)
        target_compile_definitions(alog PUBLIC ALOG_COMPILER_APPLE_CLANG=1)
    endif()
endif()

add_subdirectory(tests)
