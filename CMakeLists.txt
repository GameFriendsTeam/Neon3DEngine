cmake_minimum_required(VERSION 3.13 FATAL_ERROR)

set(PROJECT_NAME 3DEngine)

project(${PROJECT_NAME})

add_executable(${PROJECT_NAME} src/main.cpp)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

add_subdirectory(external/glfw)
target_link_libraries(${PROJECT_NAME} glfw)

include_directories(external/glm)

add_subdirectory(external/glad)
target_link_libraries(${PROJECT_NAME} glad)