cmake_minimum_required(VERSION 3.12)
project(cpptask_test VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(.. cpptask_build)

file(GLOB TEST_SOURCES "*.cpp")

add_executable(cpptask_test ${TEST_SOURCES})

find_package(Threads REQUIRED)
target_link_libraries(cpptask_test PRIVATE cpptask Threads::Threads)
target_include_directories(cpptask_test PRIVATE ../include)
