cmake_minimum_required(VERSION 3.5)

include(FetchContent)

set(CMAKE_CXX_STANDARD 17)

# Set this to true to ship the game!
option(PRODUCTION_BUILD "Make this a production build" OFF)

# Set this to false if you don’t use clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Compiler settings for Clang, MSVC, and GCC
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Release>:Release>")
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
    add_compile_options(/arch:AVX2)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # Example Clang-specific options
    add_compile_options(
        -Wall -Wextra -Wpedantic            # Warnings
        -Wno-unused-parameter              # Disable some warnings
        -ferror-limit=0                    # Show all errors
        -std=c++17                         # Use C++17
    )
    # Use libc++ if building on macOS or if needed for your platform
    if(APPLE)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
    endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    # GCC-specific options
    add_compile_options(
        -Wall -Wextra -Wpedantic            # Warnings
        -Wno-unused-parameter              # Disable some warnings
        -std=c++17                         # Use C++17
        -fexceptions                       # Enable exceptions
    )
    # Optional optimizations for GCC
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -march=native -mtune=native")
endif()

project(mygame)

# Assimp 
FetchContent_Declare(
    assimp
    GIT_REPOSITORY https://github.com/assimp/assimp.git
    GIT_TAG v5.2.5 # Use a specific release tag, or branch like "master" for latest
)
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "Disable Assimp tests" FORCE)
set(ASSIMP_NO_EXPORT ON CACHE BOOL "Disable Assimp export functionality" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build Assimp as a static library" FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "Disable Assimp tools" FORCE)
set(ASSIMP_INSTALL OFF CACHE BOOL "Disable Assimp installation" FORCE)

# FreeType
# FetchContent_Declare(
#     freetype
#     GIT_REPOSITORY https://github.com/freetype/freetype.git
#     GIT_TAG VER-2-12-1
# )
# set(FT_DISABLE_VERSION_INFO ON CACHE BOOL "" FORCE)

# Meshoptimizer
FetchContent_Declare(
    meshoptimizer
    GIT_REPOSITORY https://github.com/zeux/meshoptimizer.git
    GIT_TAG v0.18 # Use the latest stable version or a specific tag
)

FetchContent_MakeAvailable(assimp meshoptimizer)#freetype

# GLFW and other third-party dependencies
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

add_subdirectory(vendor/glfw-3.3.2)          # Window opener
add_subdirectory(vendor/glad)                # OpenGL loader
add_subdirectory(vendor/stb_image)           # Loading images
add_subdirectory(vendor/glText)              # Easy drawing text
add_subdirectory(vendor/raudio)              # Audio
add_subdirectory(vendor/glm)                 # Math
add_subdirectory(vendor/imgui-docking)       # UI
add_subdirectory(vendor/tinyobj)             # obj loader
set(RAPIDJSON_BUILD_EXAMPLES OFF CACHE BOOL "Disable RapidJSON examples" FORCE)
set(RAPIDJSON_BUILD_TESTS OFF CACHE BOOL "Disable RapidJSON tests" FORCE)
add_subdirectory(vendor/rapidjson)           # JSON Parser

# MY_SOURCES is defined to be a list of all the source files for my game
file(GLOB_RECURSE MY_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

add_executable("${CMAKE_PROJECT_NAME}")

set_property(TARGET "${CMAKE_PROJECT_NAME}" PROPERTY CXX_STANDARD 17)

target_compile_definitions("${CMAKE_PROJECT_NAME}" PUBLIC GLFW_INCLUDE_NONE=1)

if(PRODUCTION_BUILD)
    target_compile_definitions("${CMAKE_PROJECT_NAME}" PUBLIC RESOURCES_PATH="./resources/")
    target_compile_definitions("${CMAKE_PROJECT_NAME}" PUBLIC PRODUCTION_BUILD=1)
else()
    target_compile_definitions("${CMAKE_PROJECT_NAME}" PUBLIC RESOURCES_PATH="${CMAKE_CURRENT_SOURCE_DIR}/resources/")
    target_compile_definitions("${CMAKE_PROJECT_NAME}" PUBLIC PRODUCTION_BUILD=0)
endif()

target_sources("${CMAKE_PROJECT_NAME}" PRIVATE ${MY_SOURCES})

if(MSVC)
    target_compile_definitions("${CMAKE_PROJECT_NAME}" PUBLIC _CRT_SECURE_NO_WARNINGS)
    # Optional: Uncomment to remove console in Release
    # set_target_properties("${CMAKE_PROJECT_NAME}" PROPERTIES LINK_FLAGS "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # Optional: If using Clang on Windows, you might need to link against LLVM's libc++
    # target_link_options("${CMAKE_PROJECT_NAME}" PRIVATE -stdlib=libc++)
endif()

# Target link libraries
target_link_libraries("${CMAKE_PROJECT_NAME}" PRIVATE glm glfw glad stb_image raudio imgui assimp glText tinyOBJ meshoptimizer)
