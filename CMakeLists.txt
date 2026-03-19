cmake_minimum_required(VERSION 3.10)
project(ICPC_Management_System)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Create executable
add_executable(code main.cpp)

# Set output name to 'code' (required by OJ)
set_target_properties(code PROPERTIES OUTPUT_NAME "code")

# Enable optimizations for release builds
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(code PRIVATE -O2 -DNDEBUG)
endif()

# Add compiler warnings
target_compile_options(code PRIVATE -Wall -Wextra -Wpedantic)