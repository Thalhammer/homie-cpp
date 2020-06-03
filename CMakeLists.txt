cmake_minimum_required(VERSION 3.10)
project(homie-cpp)

# Testing
enable_testing()
include(GoogleTest)

find_package(ZLIB REQUIRED)

add_subdirectory(homie-cpp)
#add_subdirectory(sample_device)
#add_subdirectory(sample_master)
add_subdirectory(testbed)