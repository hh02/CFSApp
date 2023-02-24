# CMakeList.txt : CMake project for SmartSFC, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project ("SmartSFC")

# copy asset files
set(asset_directory G)
set(dst_asset_directory "${CMAKE_BINARY_DIR}/${asset_directory}")
set(src_asset_directory "${CMAKE_CURRENT_SOURCE_DIR}/${asset_directory}")

file(MAKE_DIRECTORY ${dst_asset_directory})
file(GLOB all_copy_files "${src_asset_directory}/*")
file(COPY ${all_copy_files} DESTINATION ${dst_asset_directory})

find_package(CGAL CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

file(GLOB SmartSFC_SRC
   ${CMAKE_CURRENT_SOURCE_DIR}/*.h
   ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp
   ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
)
file(GLOB ThirdParty_Header
	${CMAKE_CURRENT_SOURCE_DIR}/ThirdPartyLibs/include/*.h)

# Add source to this project's executable.
add_library(${PROJECT_NAME} STATIC ${SmartSFC_SRC} ${ThirdParty_Header})

target_link_libraries(${PROJECT_NAME} PRIVATE CGAL::CGAL)
target_link_libraries(${PROJECT_NAME} PUBLIC glm::glm)

# Compiled by visual studio 2019
target_link_libraries(${PROJECT_NAME} PRIVATE
						optimized "${CMAKE_CURRENT_SOURCE_DIR}/ThirdPartyLibs/lib/Wm5Core.lib"
						debug    "${CMAKE_CURRENT_SOURCE_DIR}/ThirdPartyLibs/lib/Wm5CoreD.lib")
target_link_libraries(${PROJECT_NAME} PRIVATE
						optimized "${CMAKE_CURRENT_SOURCE_DIR}/ThirdPartyLibs/lib/Wm5Mathematics.lib"
						debug    "${CMAKE_CURRENT_SOURCE_DIR}/ThirdPartyLibs/lib/Wm5MathematicsD.lib")

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ThirdPartyLibs/include)
# TODO: Add tests and install targets if needed.
