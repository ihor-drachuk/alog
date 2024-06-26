FILE(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS *.cpp)

set(PROJECT_TEST_NAME test-${PROJECT_NAME})

add_executable(${PROJECT_TEST_NAME} ${SOURCES})
set_property(TARGET ${PROJECT_TEST_NAME} PROPERTY CXX_STANDARD 17)
if (ALOG_HAS_QT_LIBRARY)
    set_property(TARGET ${PROJECT_TEST_NAME} PROPERTY AUTOMOC ON)
    set_property(TARGET ${PROJECT_TEST_NAME} PROPERTY AUTORCC ON)
endif()

target_link_libraries(${PROJECT_TEST_NAME} gtest gmock_main ${PROJECT_NAME})

if(ALOG_CI_SKIP_SORT_TEST)
    target_compile_definitions(${PROJECT_TEST_NAME} PRIVATE ALOG_CI_SKIP_SORT_TEST)
endif()

target_compile_options(${PROJECT_TEST_NAME} PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/WX>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Werror>
)

add_test(NAME ${PROJECT_TEST_NAME}-runner COMMAND ${PROJECT_TEST_NAME})
