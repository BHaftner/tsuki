cmake_minimum_required(VERSION 3.14)
project(MoonApp CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(BUILD_SHARED_LIBS ON CACHE BOOL "Build shared libraries" FORCE)

file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/dependencies)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/sfml)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/imgui-sfml)

add_subdirectory(dependencies)

include_directories(src)

add_compile_definitions(CPPHTTPLIB_OPENSSL_SUPPORT)

find_package(OpenSSL REQUIRED)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}")

# Add MoonInfo.cpp to the executable sources
add_executable(tsuki
    src/tsuki.cpp
    src/MoonInfo.cpp # Add MoonInfo.cpp here
)

target_include_directories(tsuki PRIVATE
    ${OPENSSL_INCLUDE_DIR}
)

# Link dependencies
target_link_libraries(tsuki PRIVATE
    OpenSSL::SSL
    OpenSSL::Crypto
    ImGui-SFML::ImGui-SFML
)

target_compile_options(tsuki PRIVATE -Wall -Wextra -Wpedantic -g)

if(WIN32 AND BUILD_SHARED_LIBS)
    add_custom_command(TARGET tsuki POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:ImGui-SFML::ImGui-SFML>
            $<TARGET_FILE:sfml-graphics>
            $<TARGET_FILE:sfml-window>
            $<TARGET_FILE:sfml-system>
            $<TARGET_FILE_DIR:tsuki>)
endif()