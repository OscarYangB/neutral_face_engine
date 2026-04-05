cmake_minimum_required(VERSION "4.2.1")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)

set(Vulkan_LIBRARY ${CMAKE_SOURCE_DIR}/external)
set(Vulkan_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/external)
find_package(Vulkan REQUIRED)

add_subdirectory("external/SDL")

project("neonatal_engine")
file(GLOB_RECURSE MY_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp")
add_executable(neonatal_engine "${MY_SOURCES}")
target_link_libraries(neonatal_engine PRIVATE Vulkan::Vulkan SDL3::SDL3-static)
