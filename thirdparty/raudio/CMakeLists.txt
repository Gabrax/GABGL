cmake_minimum_required(VERSION 3.5)
project(raudio)

cmake_policy(SET CMP0069 NEW)
add_library(raudio)
target_sources(raudio PRIVATE src/raudio.c)
target_include_directories(raudio PUBLIC include)