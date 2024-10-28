cmake_minimum_required(VERSION 3.5)
project(glad)

cmake_policy(SET CMP0069 NEW)
add_library(glad)
target_sources(glad PRIVATE src/glad.c)
target_include_directories(glad PUBLIC include)