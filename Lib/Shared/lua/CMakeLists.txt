#lua, a static library.

#Get all .cpp and .hpp files in the source directory.
file(GLOB MY_FILES RELATIVE "${CMAKE_CURRENT_LIST_DIR}" "src/*.cpp" "src/*.c" "src/*.h")

add_library(lua STATIC "${MY_FILES}")
target_include_directories(lua PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/src>
    	$<INSTALL_INTERFACE:include>
)
