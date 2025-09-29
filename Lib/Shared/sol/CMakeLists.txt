# Sol2 (header-only library)
cmake_minimum_required(VERSION 3.27)
project(sol LANGUAGES CXX)

# Sol2 is header-only, so create an INTERFACE library
add_library(sol INTERFACE)

# Headers are directly in this directory, so point to the parent
target_include_directories(sol SYSTEM INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/..)

# Link to lua
target_link_libraries(sol INTERFACE lua)

# Require C++17 or later for sol2
target_compile_features(sol INTERFACE cxx_std_17)