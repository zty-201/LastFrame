# Lua library (using precompiled binaries)
cmake_minimum_required(VERSION 3.27)
project(lua C)

# Create an imported library target
add_library(lua STATIC IMPORTED GLOBAL)

# Set the location of the .lib file
set_target_properties(lua PROPERTIES
    IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/lua54.lib"
)

# Set include directories (so other targets can find lua headers)
target_include_directories(lua INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

# Copy the DLL to the executable directory (needed at runtime on Windows)
if(WIN32)
    # Store the DLL path for later use
    set(LUA_DLL "${CMAKE_CURRENT_SOURCE_DIR}/lua54.dll" CACHE INTERNAL "")
endif()