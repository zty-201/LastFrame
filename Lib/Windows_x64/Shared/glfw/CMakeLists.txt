# glfw, a static library.

#Since glfw is already pre-compiled, it should be treated as an imported library.
add_library(glfw INTERFACE IMPORTED)

# Set the location of the library file
set_target_properties(glfw PROPERTIES IMPORTED_LOCATION
    "${CMAKE_CURRENT_SOURCE_DIR}/binary/glfw3.lib"
)
	
target_include_directories(glfw INTERFACE
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    	$<INSTALL_INTERFACE:include>
)
