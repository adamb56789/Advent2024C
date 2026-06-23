cmake_minimum_required(VERSION 3.28.3)
project(Advent2024C C)

set(CMAKE_C_STANDARD 11)

set(CMAKE_C_FLAGS_RELEASE "-O3 -fno-verbose-asm -masm=intel -march=native -g")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=native")

add_executable(Advent2024C main.c
        file-utils.h
        one.h
        two.h
        three.h
        four.h
        five.c
        five.h
        six.c
        six.h
        one.c
        two.c
        seven.c
        seven.h
        thpool.c
        thpool.h
        eight.c
        eight.h
        shared.h
)
