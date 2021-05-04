cmake_minimum_required(VERSION 3.16)
PROJECT(alog)

option(ALOG_ENABLE_TESTS     "Enable tests for ALog" OFF)
option(ALOG_ENABLE_BENCHMARK "Enable benchmark for ALog" OFF)

FILE(GLOB_RECURSE SOURCES src/*.cpp src/*.h headers/*.h)

add_library(alog STATIC ${SOURCES})
set_property(TARGET alog PROPERTY CXX_STANDARD 17)
set_property(TARGET alog PROPERTY CXX_STANDARD_REQUIRED TRUE)
target_include_directories(alog PUBLIC headers)

if(MSVC)
    target_link_options(alog PRIVATE "/ignore:4221")
    set_target_properties(alog PROPERTIES STATIC_LIBRARY_OPTIONS "/ignore:4221")
    target_compile_definitions(alog
        PRIVATE _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
        PUBLIC _CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_WARNINGS)
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    target_compile_definitions(alog PUBLIC ALOG_MACOSX)
elseif(UNIX)
    target_compile_definitions(alog PUBLIC ALOG_LINUX)
    target_link_libraries(alog PRIVATE pthread)
endif()

add_subdirectory(tests)
