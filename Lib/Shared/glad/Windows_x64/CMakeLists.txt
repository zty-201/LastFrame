# glad for windows
# Load only if windows.

if(WIN32)
	add_library(glad STATIC)
	target_sources(glad PRIVATE 
		$<$<CONFIG:DEBUG>:${CMAKE_CURRENT_LIST_DIR}/src_debug/glad.c> 
		$<$<CONFIG:RELEASE>:${CMAKE_CURRENT_LIST_DIR}/src_release/glad.c> 
	)
	target_include_directories(glad PUBLIC 
		$<$<CONFIG:DEBUG>:${CMAKE_CURRENT_LIST_DIR}/include_debug> 
		$<$<CONFIG:RELEASE>:${CMAKE_CURRENT_LIST_DIR}/include_release> 
	)
endif()