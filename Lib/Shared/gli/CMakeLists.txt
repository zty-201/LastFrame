#gli, a header only library.

add_library(gli INTERFACE)


target_include_directories(gli INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)