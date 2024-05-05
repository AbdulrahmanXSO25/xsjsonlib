cmake_minimum_required(VERSION 3.10)
project(XSJSONLIB)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Add the executable
add_executable(xsjsonlib src/main.c src/jsonhelper.c)

# Include directories for header files
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)