file(GLOB TEN_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_*.c"
)

foreach(TEST_SOURCE ${TEN_TEST_SOURCES})
    get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)

    add_executable(${TEST_NAME} ${TEST_SOURCE})

    target_link_libraries(${TEST_NAME}
        PRIVATE
            ten
    )

    target_include_directories(${TEST_NAME}
        PRIVATE
            ${PROJECT_SOURCE_DIR}/include
    )

    add_test(
        NAME ${TEST_NAME}
        COMMAND ${TEST_NAME}
    )
endforeach()
