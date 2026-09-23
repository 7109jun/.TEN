set(TEN_TEST_NAMES
    test_core
    test_error
    test_memory
    test_list
    test_map
    test_string
    test_buffer
    test_path
    test_time
    test_log
    test_file
    test_thread
    test_sync
    test_thread_pool
    test_process
    test_encoding
    test_json
    test_socket
    test_http
    test_plugin
)

foreach(test_name ${TEN_TEST_NAMES})
    add_executable(${test_name} ${test_name}.c)
    target_link_libraries(${test_name} PRIVATE ten)
    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()

# test_plugin이 ten_plugin_load()로 실제 로드해 검증할, .TEN에 링크하지
# 않는 독립된 더미 공유 라이브러리.
add_library(ten_test_plugin SHARED plugin_fixture/ten_test_plugin.c)
set_target_properties(ten_test_plugin PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED ON
)
set_tests_properties(test_plugin PROPERTIES
    ENVIRONMENT "TEN_TEST_PLUGIN_PATH=$<TARGET_FILE:ten_test_plugin>"
)
