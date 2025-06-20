message(STATUS "Configuring ImGui-SFML...")

# Configure ImGui-SFML options
set(IMGUI_DIR ${imgui_SOURCE_DIR} CACHE PATH "Path to Dear ImGui")
set(IMGUI_SFML_FIND_SFML OFF CACHE BOOL "Disable SFML search" FORCE)
set(IMGUI_SFML_IMGUI_DEMO OFF)
set(IMGUI_SFML_BUILD_EXAMPLES OFF)

# Make ImGui-SFML available
FetchContent_MakeAvailable(imgui-sfml)
