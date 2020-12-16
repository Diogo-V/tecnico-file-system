cmake_minimum_required(VERSION 3.17)
project(Project C)

set(CMAKE_C_STANDARD 99)
set(GCC_COVERAGE_COMPILE_FLAGS "-g -ansi -Wall -Wextra -pthread -lm")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${GCC_COVERAGE_COMPILE_FLAGS}" )

add_executable(Server main.c fs/operations.c fs/operations.h
        fs/state.c fs/state.h tecnicofs-api-constants.h)

add_executable(Client tecnicofs-api-constants.h client/tecnicofs-client-api.c
        client/tecnicofs-client-api.h client/tecnicofs-client.c)
