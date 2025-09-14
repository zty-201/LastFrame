#glad, a static library.

add_library(glad_debug STATIC ${CMAKE_CURRENT_LIST_DIR}/src/glad.c)

target_include_directories(glad_debug PUBLIC
    	${CMAKE_CURRENT_LIST_DIR}/include
)
