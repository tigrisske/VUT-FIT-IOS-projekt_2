cmake_minimum_required(VERSION 3.21)
project(proj2 C)

set(CMAKE_C_STANDARD 11)

include_directories(.)

add_executable(proj2
        lib.h
        proj2.c)
