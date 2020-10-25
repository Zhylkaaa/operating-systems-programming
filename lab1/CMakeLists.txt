cmake_minimum_required(VERSION 3.15)
project(PosixPthreads C)

set(CMAKE_C_STANDARD 99)

add_executable(problem1 many_readers_one_writer.c)
add_executable(problem2 many_readers_many_writers_buffer.c)