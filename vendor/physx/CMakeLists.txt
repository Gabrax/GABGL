cmake_minimum_required(VERSION 3.5)

project(PhysXLibrary)

# PhysX include and library paths
set(PHYSX_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(PHYSX_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib/release")

# PhysX libraries
set(PHYSX_LIBS
  PhysX_static_64
  PhysXCooking_static_64
  PhysXCommon_static_64
  PhysXFoundation_static_64
  PhysXPvdSDK_static_64
  PhysXTask_static_64
  PVDRuntime_64
  PhysXExtensions_static_64
  PhysXCharacterKinematic_static_64
)

# Add PhysX include directories
include_directories(${PHYSX_INCLUDE_DIR})

# Create an interface target for PhysX
add_library(PhysX INTERFACE)

target_compile_definitions(PhysX INTERFACE PX_PHYSX_STATIC_LIB)

# Link PhysX libraries to the interface target
foreach(lib IN LISTS PHYSX_LIBS)
    target_link_libraries(PhysX INTERFACE "${PHYSX_LIB_DIR}/${lib}.lib")
endforeach()

# Make include paths available for dependent targets
target_include_directories(PhysX INTERFACE ${PHYSX_INCLUDE_DIR})
