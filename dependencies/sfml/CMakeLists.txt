message(STATUS "Configuring SFML...")

# Configure SFML build options
set(SFML_BUILD_AUDIO OFF)
set(SFML_BUILD_NETWORK OFF)
set(SFML_BUILD_EXAMPLES OFF)
set(SFML_BUILD_DOC OFF)

# Make SFML available
FetchContent_MakeAvailable(sfml)
