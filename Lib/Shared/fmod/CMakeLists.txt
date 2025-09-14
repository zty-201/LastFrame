# fmod, a static library.
message("Building GLFW")
#Since fmod is already pre-compiled, it should be treated as an imported library.
#Add the keyword GLOBAL, as IMPORTED indicates local scope by default.
add_library(fmod STATIC IMPORTED GLOBAL)

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")

# Set the location of the library file
set_target_properties(glfw3 PROPERTIES
    IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/lib/x64/fmod.lib"
    #INTERFACE_LINK_LIBRARIES "${CMAKE_CURRENT_SOURCE_DIR}/binary/glfw3.lib"
    #INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Andriod")

# Set the location of the library file
set_target_properties(glfw3 PROPERTIES
    IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/binary/glfw3.lib"
    #INTERFACE_LINK_LIBRARIES "${CMAKE_CURRENT_SOURCE_DIR}/binary/glfw3.lib"
    #INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

endif()



	
target_include_directories(fmod INTERFACE
	${CMAKE_CURRENT_LIST_DIR}/include
)

set(fmod_INCLUDE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include" CACHE INTERNAL "")
set(fmod_BINARY "${CMAKE_CURRENT_SOURCE_DIR}/binary" CACHE INTERNAL "")

