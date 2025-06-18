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

if(Libnova_FOUND)
    message(STATUS "Found Libnova via find_package. Using its definitions.")
    # Libnova::Libnova is a common target name for found packages
    set(LIBNOVA_INCLUDE_DIR "${Libnova_INCLUDE_DIRS}")
    set(LIBNOVA_LIBRARY "${Libnova_LIBRARIES}")
else()
    message(STATUS "Libnova not found via find_package. Attempting manual search.")
    # Option 2: Manual search for headers and library (more robust for basic installs)
    find_path(LIBNOVA_INCLUDE_DIR NAMES libnova/lunar.h libnova/transform.h
              PATHS /usr/include /usr/local/include
              NO_CMAKE_SYSTEM_PATH # Don't search default system paths if you know it's not there
              )
    find_library(LIBNOVA_LIBRARY NAMES nova libnova
                 PATHS /usr/lib /usr/local/lib
                 NO_CMAKE_SYSTEM_PATH # Same here
                 )

    if(NOT LIBNOVA_INCLUDE_DIR)
        message(FATAL_ERROR "libnova headers not found. Please ensure libnova is installed correctly.")
    endif()
    if(NOT LIBNOVA_LIBRARY)
        message(FATAL_ERROR "libnova library not found. Please ensure libnova is installed correctly.")
    endif()
endif()

add_executable(tsuki src/tsuki.cpp)
target_compile_definitions(tsuki PRIVATE ${wxWidgets_DEFINITIONS})
target_include_directories(tsuki PRIVATE ${LIBNOVA_INCLUDE_DIR} ${wxWidgets_INCLUDE_DIRS})
target_link_libraries(tsuki PRIVATE ${LIBNOVA_LIBRARY} OpenSSL::SSL OpenSSL::Crypto ${wxWidgets_LIBRARIES})
target_compile_options(tsuki PRIVATE -Wall -Wextra -Wpedantic -g)
