#json, a header only library.

add_library(json INTERFACE)


target_include_directories(json INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/single_include>
    $<INSTALL_INTERFACE:include>
)