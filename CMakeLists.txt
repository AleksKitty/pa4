cmake_minimum_required(VERSION 3.16)
project(pa1 C)

set(CMAKE_C_STANDARD 99)
add_compile_options(-Wall -pedantic)
add_executable(pa1 main.c ipc.h common.h ipc.c pa2345.h log.h log.c lamport.h lamport.c queue.c)

find_library(libruntime libruntime.so paths ./lib64)
target_link_libraries(pa1 ${libruntime})