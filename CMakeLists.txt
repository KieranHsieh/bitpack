cmake_minimum_required(VERSION 3.10)

project(bitpack VERSION 1.0)

option(BITPACK_BUILD_TESTS OFF "Builds the unit tests for bitpack")

add_library(bitpack INTERFACE)
target_include_directories(bitpack INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(bitpack INTERFACE cxx_std_17)

if(BITPACK_BUILD_TESTS)
    message(STATUS "Building tests for Bitpack")
    add_subdirectory(tests)
endif()