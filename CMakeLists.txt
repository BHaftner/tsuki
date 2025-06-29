cmake_minimum_required(VERSION 3.14)
project(MoonApp CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(BUILD_SHARED_LIBS ON CACHE BOOL "Build shared libraries" FORCE)

include(FetchContent)

# Fetch SFML 3
FetchContent_Declare(
    sfml
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 3.0.1  # Latest stable release
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(sfml)

set(SFML_BUILD_NETWORK OFF CACHE BOOL "Build SFML network module" FORCE)
set(SFML_BUILD_EXAMPLES OFF CACHE BOOL "Build SFML examples" FORCE)
set(SFML_BUILD_DOC OFF CACHE BOOL "Build SFML documentation" FORCE)


include_directories(src)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}")

add_executable(tsuki WIN32
    src/tsuki.cpp
    src/MoonInfo.cpp
    src/simdjson.cpp
)

target_link_libraries(tsuki PRIVATE
    SFML::Graphics
    SFML::Window
    SFML::Audio
    SFML::System
)

target_compile_options(tsuki PRIVATE -Wall -Wextra -Wpedantic -g -O3 -finput-charset=UTF-8 -fexec-charset=UTF-8)

if(WIN32 AND BUILD_SHARED_LIBS)
    add_custom_command(TARGET tsuki POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:sfml-graphics>
            $<TARGET_FILE:sfml-window>
            $<TARGET_FILE:sfml-system>
            $<TARGET_FILE::sfml-audio>
            $<TARGET_FILE_DIR:tsuki>)
endif()