cmake_minimum_required(VERSION 3.16)

project(TEN
    VERSION 1.0.0
    LANGUAGES C
)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(TEN_BUILD_SHARED "Build TEN as a shared library" OFF)

set(TEN_CORE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/core")

if(TEN_BUILD_SHARED)
    add_library(ten SHARED)
else()
    add_library(ten STATIC)
endif()

target_sources(ten
    PRIVATE
        ${TEN_CORE_DIR}/src/buffer.c
        ${TEN_CORE_DIR}/src/encoding.c
        ${TEN_CORE_DIR}/src/error.c
        ${TEN_CORE_DIR}/src/file.c
        ${TEN_CORE_DIR}/src/http.c
        ${TEN_CORE_DIR}/src/json.c
        ${TEN_CORE_DIR}/src/list.c
        ${TEN_CORE_DIR}/src/log.c
        ${TEN_CORE_DIR}/src/map.c
        ${TEN_CORE_DIR}/src/memory.c
        ${TEN_CORE_DIR}/src/path.c
        ${TEN_CORE_DIR}/src/plugin.c
        ${TEN_CORE_DIR}/src/process.c
        ${TEN_CORE_DIR}/src/socket.c
        ${TEN_CORE_DIR}/src/sync.c
        ${TEN_CORE_DIR}/src/thread.c
        ${TEN_CORE_DIR}/src/thread_pool.c
        ${TEN_CORE_DIR}/src/time.c
)

target_include_directories(ten
    PUBLIC
        ${TEN_CORE_DIR}/include
)

if(WIN32)
    target_link_libraries(ten
        PRIVATE
            ws2_32
    )
else()
    find_package(Threads REQUIRED)

    target_link_libraries(ten
        PRIVATE
            Threads::Threads
    )
endif()

set_target_properties(ten PROPERTIES
    OUTPUT_NAME "ten"
)
