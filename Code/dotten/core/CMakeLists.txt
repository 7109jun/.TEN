option(TEN_BUILD_SHARED "Build .TEN as a shared library (ten.dll/libten.so) instead of static (ten.lib/libten.a)" OFF)

if (TEN_BUILD_SHARED)
    set(TEN_LIB_KIND SHARED)
else()
    set(TEN_LIB_KIND STATIC)
endif()

add_library(ten ${TEN_LIB_KIND}
    src/core/init.c
    src/core/version.c
    src/error/error.c
    src/memory/memory.c
    src/list/list.c
    src/map/map.c
    src/string/string.c
    src/buffer/buffer.c
    src/path/path.c
    src/time/time.c
    src/log/log.c
    src/thread_pool/thread_pool.c
    src/encoding/encoding.c
    src/json/json.c
    src/http/http.c
)

if (WIN32)
    target_sources(ten PRIVATE
        src/platform/windows/file_windows.c
        src/platform/windows/thread_windows.c
        src/platform/windows/sync_windows.c
        src/platform/windows/process_windows.c
        src/platform/windows/socket_windows.c
        src/platform/windows/plugin_windows.c
    )
    target_link_libraries(ten PUBLIC ws2_32)
else()
    target_sources(ten PRIVATE
        src/platform/posix/file_posix.c
        src/platform/posix/thread_posix.c
        src/platform/posix/sync_posix.c
        src/platform/posix/process_posix.c
        src/platform/posix/socket_posix.c
        src/platform/posix/plugin_posix.c
    )

    find_package(Threads REQUIRED)
    target_link_libraries(ten PUBLIC Threads::Threads ${CMAKE_DL_LIBS})
endif()

target_include_directories(ten PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# 내부 전용 헤더(src/internal/*.h)는 공개 API가 아니므로 설치되지 않고
# 라이브러리 자신의 소스 파일에서만 보이는 PRIVATE 경로로 추가한다.
target_include_directories(ten PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

set_target_properties(ten PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED ON
    C_EXTENSIONS OFF
    # 공유 라이브러리 + Windows일 때, 모든 공개 헤더에 일일이
    # __declspec(dllexport)/__declspec(dllimport)를 붙이지 않고도 모든
    # public 심볼을 자동으로 내보내게 하는 CMake 3.x 기능. MinGW는
    # 원래 기본적으로 심볼을 다 내보내므로 이 옵션은 주로 MSVC를 위함.
    WINDOWS_EXPORT_ALL_SYMBOLS ON
)

if (MSVC)
    target_compile_options(ten PRIVATE /W4)
else()
    target_compile_options(ten PRIVATE -Wall -Wextra -Wpedantic)
endif()

install(TARGETS ten
    ARCHIVE DESTINATION lib   # 정적 라이브러리(ten.lib / libten.a)
    LIBRARY DESTINATION lib   # 공유 라이브러리(libten.so / libten.dylib)
    RUNTIME DESTINATION bin   # Windows의 ten.dll (import lib는 ARCHIVE로 감)
)
install(DIRECTORY include/ten DESTINATION include)

if (TEN_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if (TEN_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
