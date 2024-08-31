cmake_minimum_required(VERSION 3.5)

project(psvrplayer LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(${PROJECT_NAME} main.cpp glad/src/glad.c)

target_include_directories(${PROJECT_NAME} PRIVATE "glad/include" ".")
target_link_libraries(${PROJECT_NAME} glfw)
