cmake_minimum_required(VERSION 3.5)
project(tinyOBJ)

# Set CMake policy for CMP0069, if necessary
cmake_policy(SET CMP0069 NEW)

# Define an interface library
add_library(tinyOBJ INTERFACE)

# Specify the absolute path for the header file
target_sources(tinyOBJ INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/src/tiny_obj_loader.h")

# Specify include directories so that any target linking to glText can find the header
target_include_directories(tinyOBJ INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/src")