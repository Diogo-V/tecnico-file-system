cmake_minimum_required(VERSION 3.17)
project(Projeto_EX1 C)

set(CMAKE_C_STANDARD 99)
set(GCC_COVERAGE_COMPILE_FLAGS "-g -ansi -Wall -Wextra -pthread -lm")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${GCC_COVERAGE_COMPILE_FLAGS}" )

add_executable(Projeto_EX1 main.c fs/operations.c fs/operations.h
        fs/state.c fs/state.h tecnicofs-api-constants.h)
