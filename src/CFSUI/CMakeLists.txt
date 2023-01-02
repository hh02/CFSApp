project(CFSUI)



add_executable(app main.cpp)

# Add project files
file(GLOB SRC_FILES *.cpp)
add_executable(${PROJECT_NAME} ${SRC_FILES})
target_link_libraries(${PROJECT_NAME} PUBLIC igl::imgui)
