cmake_minimum_required(VERSION 3.5)

project(psvrplayer LANGUAGES C CXX)
project(${PROJECT_NAME} VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")

set(SOURCE_FILES
  "framepool.cpp"
  "main.cpp"
  "monitors.cpp"
  "play_screen.cpp"
  "transformer.cpp"
  "video_player.cpp"
  "vr_helmet.cpp"
  "glad/src/glad.c"
  )

set(HEADER_FILES
  "framepool.h"
  "monitors.h"
  "play_screen.h"
  "transformer.h"
  "version.h"
  "video_player.h"
  "vr_helmet.h"
  "shaders/output.h"
  )

# Additional files. They aren't used in compilation, but showed in project
set(AUX_FILES
  "shaders/output.frag"
  "shaders/output.vert"
  )

find_package(LIBVLC REQUIRED)
include_directories(${LIBVLC_INCLUDE_DIR})

find_package(HIDAPI REQUIRED)
include_directories(${HIDAPI_INCLUDE_DIRS})

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package (Threads REQUIRED)


add_executable(${PROJECT_NAME} ${SOURCE_FILES} ${HEADER_FILES} ${AUX_FILES})

target_include_directories(${PROJECT_NAME} PRIVATE "glad/include" ".")
target_link_libraries(${PROJECT_NAME} glfw)
target_link_libraries(${PROJECT_NAME} ${LIBVLC_LIBRARY})
target_link_libraries(${PROJECT_NAME} ${HIDAPI_LIBRARIES})
target_link_libraries(${PROJECT_NAME} Threads::Threads)
