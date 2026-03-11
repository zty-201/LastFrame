# Compressonator, a static texture compression library.
if(WIN32)
message("Linking Compressonator")
#Since glfw is already pre-compiled, it should be treated as an imported library.
#Add the keyword GLOBAL, as IMPORTED indicates local scope by default.
add_library(compressonator STATIC IMPORTED GLOBAL)

# Set the location of the library file
set_target_properties(compressonator PROPERTIES
    IMPORTED_LOCATION_DEBUG "${CMAKE_CURRENT_SOURCE_DIR}/lib/bin/x64/Compressonator_MD.lib"
    IMPORTED_LOCATION_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/lib/bin/x64/Compressonator_MD.lib"
    #INTERFACE_LINK_LIBRARIES "${CMAKE_CURRENT_SOURCE_DIR}/binary/glfw3.lib"
    #INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

	
target_include_directories(compressonator INTERFACE
	${CMAKE_CURRENT_LIST_DIR}/include
)

# set(GLFW_INCLUDE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include" CACHE INTERNAL "")
# set(GLFW_BINARY "${CMAKE_CURRENT_SOURCE_DIR}/binary" CACHE INTERNAL "")
endif()

