cmake_minimum_required(VERSION 3.20)
project(MCYEngine)

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 17)

# Vulkan SDK from environment variable
include_directories($ENV{VULKAN_SDK}/Include)

# Project includes
include_directories(
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/src/vendor
)

# Let CLion see all source files
file(GLOB_RECURSE SRC
    src/*.cpp
    src/*.c
    src/*.h
)

add_executable(dummy ${SRC})