cmake_minimum_required(VERSION 3.16)
project(aethe LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(aethe main.cpp)
find_package(Threads REQUIRED)
target_compile_definitions(aethe PRIVATE AETHE_ENABLE_THREADS=1)
target_link_libraries(aethe PRIVATE Threads::Threads)
