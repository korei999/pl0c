cmake_minimum_required(VERSION 3.8)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_COLOR_DIAGNOSTICS ON)
set(CMAKE_C_STANDARD 23)
set(CMAKE_C_EXTENSIONS OFF)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

project(pl0c VERSION 0.1 LANGUAGES C)

add_compile_options(-Wall -Wextra -Werror=incompatible-pointer-types)
add_compile_definitions("-DLOGS") # used for LOG_* and COUT/CERR macros
add_compile_definitions("-D_DEFAULT_SOURCE") # needed for reallocarray
include_directories(BEFORE "include")

add_executable(
    pl0c
    "src/main.c"
    "src/token.c"
)

if (CMAKE_BUILD_TYPE MATCHES "Asan")
    set(CMAKE_BUILD_TYPE "Debug")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=undefined -fsanitize=address")
    add_link_options(-fsanitize=undefined -fsanitize=address)
endif()

if (CMAKE_BUILD_TYPE MATCHES "Debug")
    target_compile_options(pl0c PRIVATE -Wno-unused-parameter -Wno-unused-variable -Wno-unused-function)
endif()

find_library(LIB_BSD bsd REQUIRED)
if (LIB_BSD)
    target_link_options(pl0c PRIVATE ${LIB_BSD})
endif()

message(STATUS "CMAKE_BUILD_TYPE: '${CMAKE_BUILD_TYPE}'")
