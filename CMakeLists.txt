cmake_minimum_required(VERSION 3.22.1)

project(stdnet)

add_subdirectory(stdexec)
include_directories(include)

add_executable(main main.cpp)
target_link_libraries(main STDEXEC::stdexec)
