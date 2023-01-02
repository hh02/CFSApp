cmake_minimum_required(VERSION 3.16)
project(CFSApp)

list(PREPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# Libigl
include(libigl)

# Enable the target igl::imgui
igl_include(imgui)

add_subdirectory(thirdparty)
add_subdirectory(src)


