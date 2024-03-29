file(GLOB SOURCES RELATIVE CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR} *.cpp)

foreach( testsourcefile ${SOURCES} )
    string( REPLACE ".cpp" "" testname ${testsourcefile} )

    add_executable( benchmark-${testname} ${testsourcefile} )
    set_property(TARGET benchmark-${testname} PROPERTY CXX_STANDARD 17)
    target_link_libraries(benchmark-${testname} gtest benchmark alog)

    add_test(NAME benchmark-${testname}-runner COMMAND benchmark-${testname})

    target_compile_options(benchmark-${testname} PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/WX>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Werror>
    )
endforeach( testsourcefile ${APP_SOURCES} )
