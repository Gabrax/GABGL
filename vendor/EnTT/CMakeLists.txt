cmake_minimum_required(VERSION 3.5)
project(EnTT)

cmake_policy(SET CMP0069 NEW)

add_library(EnTT INTERFACE)

target_sources(EnTT INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/src/entt.hpp")

target_include_directories(EnTT INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/src")