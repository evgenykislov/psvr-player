cmake_minimum_required(VERSION 3.5)

project(psvrplayer LANGUAGES C CXX)
project(${PROJECT_NAME} VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")

set(SOURCE_FILES
  "frame_buffer.cpp"
  "framepool.cpp"
  "main.cpp"
  "monitors.cpp"
  "play_screen.cpp"
  "playing.cpp"
  "shader_program.cpp"
  "transformer.cpp"
  "video_player.cpp"
  "vr_helmet.cpp"
  "glad/src/glad.c"
  )

set(HEADER_FILES
  "frame_buffer.h"
  "framepool.h"
  "monitors.h"
  "play_screen.h"
  "playing.h"
  "shader_program.h"
  "transformer.h"
  "version.h"
  "video_player.h"
  "vr_helmet.h"
  )

set(COMPILED_RESOURCES
  "shaders/halfcilinder.frag"
  "shaders/halfcilinder.vert"
  "shaders/output.frag"
  "shaders/output.vert"
  "shaders/split.frag"
  "shaders/split.vert"
  )

find_package(LIBVLC REQUIRED)
include_directories(${LIBVLC_INCLUDE_DIR})

find_package(HIDAPI REQUIRED)
include_directories(${HIDAPI_INCLUDE_DIRS})

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package (Threads REQUIRED)

# Resource compiler
SET(RESOURCE_COMPILER xxd)
FOREACH(INPUT_FILE ${COMPILED_RESOURCES})
  SET(OUTPUT_FILE ${INPUT_FILE}.h)
  ADD_CUSTOM_COMMAND(
    OUTPUT ${OUTPUT_FILE}
    COMMAND ${RESOURCE_COMPILER} -i ${INPUT_FILE} ${OUTPUT_FILE}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Compiling ${INPUT_FILE} to binary resource")
  LIST(APPEND COMPILED_RESOURCES ${OUTPUT_FILE})
ENDFOREACH()

add_executable(${PROJECT_NAME} ${SOURCE_FILES} ${HEADER_FILES} ${COMPILED_RESOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE "glad/include" ".")
target_link_libraries(${PROJECT_NAME} glfw)
target_link_libraries(${PROJECT_NAME} ${LIBVLC_LIBRARY})
target_link_libraries(${PROJECT_NAME} ${HIDAPI_LIBRARIES})
target_link_libraries(${PROJECT_NAME} Threads::Threads)
