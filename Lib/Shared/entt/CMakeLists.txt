#entt, a header only library.

add_library(entt INTERFACE)


target_include_directories(entt INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:include>
)