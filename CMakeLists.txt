cmake_minimum_required(VERSION 3.16)
PROJECT(alog)

option(ALOG_ENABLE_TESTS             "ALog: Enable tests" OFF)
option(ALOG_ENABLE_BENCHMARK         "ALog: Enable benchmark" OFF)
option(ALOG_ENABLE_DEF_SEPARATORS    "ALog: Enable space-separators by default" OFF)
option(ALOG_ENABLE_DEF_AUTO_QUOTES   "ALog: Enable auto quotes" ON)
option(ALOG_ENABLE_DEBUG             "ALog: Enable additional debug checks" OFF)

FILE(GLOB_RECURSE SOURCES src/*.cpp src/*.h headers/*.h)

add_library(alog STATIC ${SOURCES})
target_compile_features(alog PUBLIC cxx_std_17)
target_include_directories(alog PUBLIC headers)

find_package(QT NAMES Qt6 Qt5 OPTIONAL_COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} QUIET OPTIONAL_COMPONENTS Core)

if(TARGET p7)
    target_link_libraries(alog PRIVATE p7)
    target_compile_definitions(alog PUBLIC ALOG_HAS_P7_LIBRARY)
endif()

if(Qt5_FOUND OR Qt6_FOUND)
    target_link_libraries(alog PUBLIC Qt${QT_VERSION_MAJOR}::Core)
    target_compile_definitions(alog PUBLIC ALOG_HAS_QT_LIBRARY)
endif()

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

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    target_compile_definitions(alog PUBLIC ALOG_MACOSX)
elseif(UNIX)
    target_compile_definitions(alog PUBLIC ALOG_LINUX)
    target_link_libraries(alog PRIVATE pthread)
endif()

add_subdirectory(tests)
