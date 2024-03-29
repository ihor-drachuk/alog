set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(test-alog-2 test-alog-2-main.cpp)
set_property(TARGET test-alog-2 PROPERTY CXX_STANDARD 17)
target_link_libraries(test-alog-2 PRIVATE alog)

target_compile_options(test-alog-2 PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/WX>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Werror>
)

add_test(NAME test-alog-2-runner COMMAND test-alog-2)
