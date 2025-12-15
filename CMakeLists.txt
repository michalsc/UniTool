cmake_minimum_required(VERSION 3.14.0)
project(UniTool VERSION 1.0.0)

include(cmake/verstring.cmake)
get_verstring(VERSTRING)

if(NOT TARGET unicam)
    add_subdirectory(unicam.resource EXCLUDE_FROM_ALL)
endif()

add_executable(UniTool
    src/main.c
)

target_link_options(UniTool PRIVATE -ffreestanding -m68040 -s -nostdlib -nostartfiles -Wl,-e,__start)
target_compile_options(UniTool PRIVATE -Os -m68040 -m68881 -fomit-frame-pointer)
target_compile_definitions(UniTool PRIVATE VERSION_STRING="${VERSTRING}")
target_include_directories(UniTool PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(UniTool mui amiga unicam)

install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/UniTool DESTINATION UniTool/)
