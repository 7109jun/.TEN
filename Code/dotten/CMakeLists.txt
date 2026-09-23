cmake_minimum_required(VERSION 3.15)
project(ten LANGUAGES C)

option(TEN_BUILD_EXAMPLES "Build .TEN example programs" ON)
option(TEN_BUILD_TESTS "Build .TEN test suite" ON)

add_subdirectory(core)
