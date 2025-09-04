cmake_minimum_required(VERSION 3.31)
project(Advent2024C C)

set(CMAKE_C_STANDARD 99)

set(CMAKE_C_FLAGS_RELEASE "-O3 -g -fno-verbose-asm -masm=intel -march=native")

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
)
