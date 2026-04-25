cmake_minimum_required(VERSION 3.16)
project(aethe LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
include(CheckIPOSupported)

option(AETHE_ENABLE_IPO "Enable interprocedural optimization for release builds" ON)

add_executable(aethe main.cpp)
find_package(Threads REQUIRED)
target_compile_definitions(aethe PRIVATE AETHE_ENABLE_THREADS=1)

if(MSVC)
    target_compile_options(aethe PRIVATE /W4 /permissive-)
else()
    target_compile_options(aethe PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        $<$<CONFIG:Release>:-O3>
        $<$<CONFIG:RelWithDebInfo>:-O2>
    )
endif()

if(AETHE_ENABLE_IPO)
    check_ipo_supported(RESULT ipo_supported OUTPUT ipo_output)
    if(ipo_supported)
        set_property(TARGET aethe PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
        set_property(TARGET aethe PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO TRUE)
    endif()
endif()

target_link_libraries(aethe PRIVATE Threads::Threads)
