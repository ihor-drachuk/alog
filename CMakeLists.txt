cmake_minimum_required(VERSION 3.16)
PROJECT(alog)

option(ALOG_ENABLE_TESTS             "ALog: Enable tests" OFF)
option(ALOG_ENABLE_BENCHMARK         "ALog: Enable benchmark" OFF)
option(ALOG_ENABLE_DEF_SEPARATORS    "ALog: Enable space-separators by default" ON)
option(ALOG_ENABLE_DEF_AUTO_QUOTES   "ALog: Enable auto quotes" ON)
option(ALOG_ENABLE_DEBUG             "ALog: Enable additional debug checks" OFF)

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

target_compile_options(alog PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/WX>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Werror>
)

if(ALOG_ENABLE_DEF_SEPARATORS)
    target_compile_definitions(alog PUBLIC ALOG_ENABLE_DEF_SEPARATORS)
endif()

if(ALOG_ENABLE_DEF_AUTO_QUOTES)
    target_compile_definitions(alog PUBLIC ALOG_ENABLE_DEF_AUTO_QUOTES)
endif()

if(ALOG_ENABLE_DEBUG)
    target_compile_definitions(alog PUBLIC ALOG_ENABLE_DEBUG)
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    target_compile_definitions(alog PUBLIC ALOG_MACOSX)
elseif(UNIX)
    target_compile_definitions(alog PUBLIC ALOG_LINUX)
    target_link_libraries(alog PRIVATE pthread)
endif()

add_subdirectory(tests)
