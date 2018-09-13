cmake_minimum_required(VERSION 3.10)
project(creation_tk C)

set(CMAKE_C_STANDARD 11)

SET(PL_SUPPORT_OPENGL OFF CACHE BOOL "Graphics OpenGL API support")

add_subdirectory(platform/platform)

include_directories(platform/platform/include/)

add_executable(creation_tk main.c types.h)

target_link_libraries(creation_tk platform)