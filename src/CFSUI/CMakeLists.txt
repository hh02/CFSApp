project(CFSUI)


set(CMAKE_CXX_STANDARD 17)
# Add project files
file(GLOB SRC_FILES *.cpp *.c *.h *.hpp *.inl)
add_executable(${PROJECT_NAME} ${SRC_FILES})
target_link_libraries(${PROJECT_NAME} PUBLIC igl::imgui)
target_link_libraries(${PROJECT_NAME} PUBLIC SmartSFC)
