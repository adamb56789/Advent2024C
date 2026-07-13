cmake_minimum_required(VERSION 3.28.3)
project(Advent2024C C)

set(CMAKE_C_STANDARD 11)

set(CMAKE_C_FLAGS_RELEASE "-O3 -march=native -g")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=native")

add_executable(Advent2024C main.c
        platform.h
        shared.h
        dumb_lil_threadpool.c
        dumb_lil_threadpool.h
        1.c
        1.h
        2.c
        2.h
        3.c
        3.h
        4.c
        4.h
        5.c
        5.h
        6.c
        6.h
        6_parallel.c
        6_parallel.h
        7.c
        7.h
        8.c
        8.h
)
