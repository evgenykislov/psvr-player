cmake_minimum_required(VERSION 3.14)

project(unittests)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SOURCE_FILES
  "rotation_view.cpp"
  "../psvrplayer/rotation.cpp"
)

set(HEADER_FILES
  "../psvrplayer/rotation.h"
)

find_package(GTest REQUIRED)
find_package(Threads REQUIRED)

include_directories(${GTEST_INCLUDE_DIR})

add_executable(${PROJECT_NAME} ${HEADER_FILES} ${SOURCE_FILES})

target_link_libraries(${PROJECT_NAME} ${GTEST_LIBRARY} ${GTEST_MAIN_LIBRARY})
target_link_libraries(${PROJECT_NAME} Threads::Threads)
