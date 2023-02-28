project(CFSUI)


set(CMAKE_CXX_STANDARD 17)

find_package(Freetype REQUIRED)
# Add project files
file(GLOB SRC_FILES *.cpp *.c *.h *.hpp *.inl)
add_executable(${PROJECT_NAME} ${SRC_FILES})
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(${PROJECT_NAME} PUBLIC Freetype::Freetype)
target_link_libraries(${PROJECT_NAME} PUBLIC igl::imgui)
target_link_libraries(${PROJECT_NAME} PUBLIC third_party)
target_link_libraries(${PROJECT_NAME} PUBLIC SmartSFC)
