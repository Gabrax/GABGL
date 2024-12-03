cmake_minimum_required(VERSION 3.5)
project(tinyOBJ)

cmake_policy(SET CMP0069 NEW)

add_library(tinyOBJ INTERFACE)

target_sources(tinyOBJ INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/src/tiny_obj_loader.h")

target_include_directories(tinyOBJ INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/src")