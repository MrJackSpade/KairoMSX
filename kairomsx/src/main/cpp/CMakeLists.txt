cmake_minimum_required(VERSION 3.22.1)
project(kairomsx LANGUAGES C CXX)

get_filename_component(KAIROMSX_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../.." ABSOLUTE)
include("${KAIROMSX_ROOT}/shared/cmake/KairoMachine.cmake")

# The MSX on openMSX, driven one frame at a time (core/).
add_subdirectory("${KAIROMSX_ROOT}/core" core)

kairo_machine_library(kairomsx msx_core.cpp)
target_link_libraries(kairomsx PRIVATE kairomsx_core)
