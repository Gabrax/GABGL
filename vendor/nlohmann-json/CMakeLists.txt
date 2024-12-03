cmake_minimum_required(VERSION 3.5)
project(JSONparser)

cmake_policy(SET CMP0069 NEW)

add_library(JSONparser INTERFACE)

target_sources(JSONparser INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/src/json.hpp")

target_include_directories(JSONparser INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/src")