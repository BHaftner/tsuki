cmake_minimum_required(VERSION 3.14)
project(MoonApp CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Force dynamic linking for all dependencies
set(BUILD_SHARED_LIBS ON CACHE BOOL "Build shared libraries" FORCE)

# Create dependencies management structure
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/dependencies)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/sfml)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/imgui-sfml)

# Add dependencies directory
add_subdirectory(dependencies)

include_directories(src)

# Add OpenSSL and libnova
add_compile_definitions(CPPHTTPLIB_OPENSSL_SUPPORT)

find_package(Libnova QUIET)
find_package(OpenSSL REQUIRED)

if(Libnova_FOUND)
    message(STATUS "Found Libnova via find_package.")
    set(LIBNOVA_INCLUDE_DIR "${Libnova_INCLUDE_DIRS}")
    set(LIBNOVA_LIBRARY "${Libnova_LIBRARIES}")
else()
    find_path(LIBNOVA_INCLUDE_DIR NAMES libnova/lunar.h libnova/transform.h)
    find_library(LIBNOVA_LIBRARY NAMES nova libnova)
    if(NOT LIBNOVA_INCLUDE_DIR)
        message(FATAL_ERROR "libnova headers not found.")
    endif()
    if(NOT LIBNOVA_LIBRARY)
        message(FATAL_ERROR "libnova library not found.")
    endif()
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}")

add_executable(tsuki src/tsuki.cpp)
target_include_directories(tsuki PRIVATE
    ${LIBNOVA_INCLUDE_DIR}
    ${OPENSSL_INCLUDE_DIR}
)

# Link dependencies
target_link_libraries(tsuki PRIVATE
    ${LIBNOVA_LIBRARY}
    OpenSSL::SSL
    OpenSSL::Crypto
    ImGui-SFML::ImGui-SFML
)

target_compile_options(tsuki PRIVATE -Wall -Wextra -Wpedantic -g)

# Windows DLL copy for dynamic linking
if(WIN32 AND BUILD_SHARED_LIBS)
    add_custom_command(TARGET tsuki POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:ImGui-SFML::ImGui-SFML>
            $<TARGET_FILE:sfml-graphics>
            $<TARGET_FILE:sfml-window>
            $<TARGET_FILE:sfml-system>
            $<TARGET_FILE_DIR:tsuki>)
endif()