#glad, a static library.

add_library(glad_release STATIC ${CMAKE_CURRENT_LIST_DIR}/src/glad.c)

target_include_directories(glad_release PUBLIC
    	${CMAKE_CURRENT_LIST_DIR}/include)
