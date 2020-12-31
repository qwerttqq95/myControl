cmake_minimum_required(VERSION 3.14)
project(untitled1)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
link_libraries(ws2_32)
set(CMAKE_EXE_LINKER_FLAGS "-static")
aux_source_directory(. SOURCE)

add_executable(untitled1 main.cpp myH.h )





