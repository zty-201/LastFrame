#xresource, a header only library.

add_library(xresource INTERFACE)


target_include_directories(xresource SYSTEM INTERFACE 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)