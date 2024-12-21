cmake_minimum_required(VERSION 3.5)

include(FetchContent)

set(CMAKE_CXX_STANDARD 17)

# Set this to true to ship the game!
option(PRODUCTION_BUILD "Make this a production build" OFF)

# Set this to false if you don’t use clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Compiler settings for Clang, MSVC, and GCC
if(MSVC)
    # Set the MSVC runtime library to static multithreading for Release configuration
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:RelWithDebInfo>:RelWithDebInfo>")
    add_compile_options(/MP)

    file(COPY
      "${CMAKE_BINARY_DIR}/_deps/assimp-build/bin/assimp-vc143-mt.dll"
      "${CMAKE_SOURCE_DIR}/vendor/physx/lib/release/PhysX_64.dll"
      "${CMAKE_SOURCE_DIR}/vendor/physx/lib/release/PhysXCommon_64.dll"
      "${CMAKE_SOURCE_DIR}/vendor/physx/lib/release/PhysXCooking_64.dll"
      "${CMAKE_SOURCE_DIR}/vendor/physx/lib/release/PhysXFoundation_64.dll"
      "${CMAKE_SOURCE_DIR}/vendor/physx/lib/release/PVDRuntime_64.dll"

       DESTINATION "${CMAKE_BINARY_DIR}"
    )

elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_compile_options(
        # -Wall -Wextra -Wpedantic
        -Wno-unused-function
        -Wno-unused-parameter
        -ferror-limit=0
        -std=c++17
    )
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    add_compile_options(
        -Wall -Wextra -Wpedantic
        -Wno-unused-function
        -Wno-unused-parameter
        -std=c++17
        -fexceptions 
    )
endif()

project(gl_engine)

# Assimp 
FetchContent_Declare(
    assimp
    GIT_REPOSITORY https://github.com/assimp/assimp.git
    GIT_TAG v5.3.0
)
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "Disable Assimp tests" FORCE)

# Meshoptimizer
FetchContent_Declare(
    meshoptimizer
    GIT_REPOSITORY https://github.com/zeux/meshoptimizer.git
    GIT_TAG v0.18
)

# TracyProfiler
#FetchContent_Declare (
#    tracy
#    GIT_REPOSITORY https://github.com/wolfpld/tracy.git
#    GIT_TAG v0.11.1
#)

# ImGuizmo
FetchContent_Declare(
    imguizmo
    GIT_REPOSITORY https://github.com/CedricGuillemet/ImGuizmo
    GIT_TAG master
)

# spdlog
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.11.0 
)

FetchContent_MakeAvailable(assimp meshoptimizer imguizmo spdlog)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

add_subdirectory(vendor/glfw-3.3.2)
add_subdirectory(vendor/glad)
add_subdirectory(vendor/stb_image)
add_subdirectory(vendor/glText)
add_subdirectory(vendor/raudio)
add_subdirectory(vendor/glm)
add_subdirectory(vendor/imgui-docking)
add_subdirectory(vendor/tinyobj)
add_subdirectory(vendor/EnTT)
add_subdirectory(vendor/nlohmann-json)
add_subdirectory(vendor/physx)

file(GLOB_RECURSE MY_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

add_executable("${CMAKE_PROJECT_NAME}")

set_property(TARGET "${CMAKE_PROJECT_NAME}" PROPERTY CXX_STANDARD 17)

file(GLOB IMGUIMO_SRC "${imguizmo_SOURCE_DIR}/ImGuizmo.cpp")
file(GLOB IMGUIMO_HEADERS "${imguizmo_SOURCE_DIR}/ImGuizmo.h")

target_include_directories("${CMAKE_PROJECT_NAME}" PRIVATE ${imguizmo_SOURCE_DIR})

target_compile_definitions("${CMAKE_PROJECT_NAME}" PUBLIC GLFW_INCLUDE_NONE=1)


target_sources("${CMAKE_PROJECT_NAME}" PRIVATE ${MY_SOURCES} ${IMGUIMO_SRC})

if(MSVC)
    target_compile_definitions("${CMAKE_PROJECT_NAME}" PUBLIC _CRT_SECURE_NO_WARNINGS)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # Optional: If using Clang on Windows, you might need to link against LLVM's libc++
    # target_link_options("${CMAKE_PROJECT_NAME}" PRIVATE -stdlib=libc++)
endif()

option(RELEASE "Enable Windows subsystem for the application" OFF)

if(RELEASE)
    set_target_properties("${CMAKE_PROJECT_NAME}" PROPERTIES LINK_FLAGS "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")
endif()

# Target link libraries
target_link_libraries("${CMAKE_PROJECT_NAME}" PRIVATE glm glfw glad stb_image raudio imgui assimp glText tinyOBJ meshoptimizer EnTT JSONparser PhysX spdlog::spdlog)
