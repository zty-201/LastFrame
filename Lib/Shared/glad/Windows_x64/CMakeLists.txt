# glad for windows
# Load only if windows.

if(WIN32)
	add_library(glad STATIC)
	target_sources(glad PRIVATE 
		$<$<CONFIG:DEBUG>:"${CMAKE_CURRENT_LIST_DIR}/glad_debug/src/glad.c"> 
		$<$<CONFIG:RELEASE>:"${CMAKE_CURRENT_LIST_DIR}/glad_release/src/glad.c"> 
	)
	target_include_directories(glad PUBLIC 
		$<$<CONFIG:DEBUG>:"${CMAKE_CURRENT_LIST_DIR}/glad_debug/include"> 
		$<$<CONFIG:RELEASE>:"${CMAKE_CURRENT_LIST_DIR}/glad_release/include"> 
	)
endif()