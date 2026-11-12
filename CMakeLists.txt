cmake_minimum_required(VERSION 3.20)
project(Project3Variant8 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

find_package(Threads REQUIRED)

add_executable(prog src/main.cpp)
target_link_libraries(prog PRIVATE Threads::Threads)

if(MINGW)
    target_link_options(prog PRIVATE -static)
endif()

if(MSVC)
    target_compile_options(prog PRIVATE /W4 /utf-8)
else()
    target_compile_options(prog PRIVATE -Wall -Wextra -Wpedantic)
endif()
