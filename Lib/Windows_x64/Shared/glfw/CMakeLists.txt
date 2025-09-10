# glfw, a static library.

add_library(glfw STATIC ${CMAKE_CURRENT_SOURCE_DIR}/binary/glfw3.lib)
	
target_include_directories(glfw PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    	$<INSTALL_INTERFACE:include>
)
