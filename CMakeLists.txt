cmake_minimum_required(VERSION 3.10)
project(MoonApp CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include_directories(src)

add_compile_definitions(CPPHTTPLIB_OPENSSL_SUPPORT)

find_package(Libnova QUIET)
find_package(OpenSSL REQUIRED)
find_package(wxWidgets COMPONENTS core base gl REQUIRED)

if(Libnova_FOUND) # This check is still valid if a FindLibnova.cmake module *is* found (even if not a config file)
    message(STATUS "Found Libnova via find_package. Using its definitions.")
    set(LIBNOVA_INCLUDE_DIR "${Libnova_INCLUDE_DIRS}")
    set(LIBNOVA_LIBRARY "${Libnova_LIBRARIES}")
else()
    message(STATUS "Libnova not found via find_package. Attempting manual search.")
    find_path(LIBNOVA_INCLUDE_DIR NAMES libnova/lunar.h libnova/transform.h) # Rely on CMake's default search paths
    find_library(LIBNOVA_LIBRARY NAMES nova libnova) # Rely on CMake's default search paths

    if(NOT LIBNOVA_INCLUDE_DIR)
        message(FATAL_ERROR "libnova headers not found. Please ensure libnova is installed correctly.")
    endif()
    if(NOT LIBNOVA_LIBRARY)
        message(FATAL_ERROR "libnova library not found. Please ensure libnova is installed correctly.")
    endif()
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}")

add_executable(tsuki src/tsuki.cpp)
target_compile_definitions(tsuki PRIVATE ${wxWidgets_DEFINITIONS})
target_include_directories(tsuki PRIVATE ${LIBNOVA_INCLUDE_DIR} ${wxWidgets_INCLUDE_DIRS})
target_link_libraries(tsuki PRIVATE ${LIBNOVA_LIBRARY} OpenSSL::SSL OpenSSL::Crypto ${wxWidgets_LIBRARIES})
target_compile_options(tsuki PRIVATE -Wall -Wextra -Wpedantic -g)
