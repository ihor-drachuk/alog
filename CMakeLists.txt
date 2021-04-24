cmake_minimum_required(VERSION 3.16)
PROJECT(alog)

option(ALOG_ENABLE_TESTS     "Enable tests for ALog" OFF)
option(ALOG_ENABLE_BENCHMARK "Enable benchmark for ALog" OFF)

FILE(GLOB_RECURSE SOURCES src/*.cpp src/*.h headers/*.h)

add_library(alog STATIC ${SOURCES})
set_property(TARGET alog PROPERTY CXX_STANDARD 17)
target_include_directories(alog PUBLIC headers)

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    target_compile_definitions(alog PUBLIC ALOG_MACOSX)
elseif(UNIX)
    target_compile_definitions(alog PUBLIC ALOG_LINUX)
endif()

add_subdirectory(tests)
