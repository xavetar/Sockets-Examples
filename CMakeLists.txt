cmake_minimum_required(VERSION 3.0...999999.0)
project(AF_LU C)

set(CMAKE_C_STANDARD 23)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/Build)

add_subdirectory(Code/Network/C/Sockets)
