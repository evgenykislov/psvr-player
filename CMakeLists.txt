cmake_minimum_required(VERSION 3.18)

project(psvr-player)

set(LIBINIPARSER_DISABLE_INSTALL ON CACHE BOOL "Disable installation of iniparser library")

add_subdirectory(libs/iniparser)
add_subdirectory(psvrplayer)
