cmake_minimum_required(VERSION 3.14)
project(railway-interlock-validator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

include_directories(${PROJECT_SOURCE_DIR}/include)

file(GLOB_RECURSE SOURCES
    ${PROJECT_SOURCE_DIR}/src/*.cpp
)

add_executable(ilvalidator ${SOURCES})

target_include_directories(ilvalidator PRIVATE
    ${PROJECT_SOURCE_DIR}/include
)

enable_testing()
