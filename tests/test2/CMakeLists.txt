cmake_minimum_required(VERSION 3.16)

project(test-alog-2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(test-alog-2 main.cpp)
target_link_libraries(test-alog-2 PRIVATE alog)

target_compile_options(test-alog-2 PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/WX>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Werror>
)
