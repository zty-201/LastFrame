#imgui, a static library.

#Get all .cpp and .hpp files in the current directory.
file(GLOB MY_FILES RELATIVE "${CMAKE_CURRENT_LIST_DIR}" "*.cpp" "*.h")
file(GLOB MY_BACKENDS RELATIVE "${CMAKE_CURRENT_LIST_DIR}" "backends/*.cpp" "backends/*.h")
#Note that imgui.natvis and imgui.natstepfilter is visual studio specific (and not necessary). Might have to change this.
file(GLOB MISC_FILES RELATIVE "${CMAKE_CURRENT_LIST_DIR}" "misc/cpp/imgui_stdlib.*" "misc/debuggers/imgui.natvis" "misc/debuggers/imgui.natstepfilter")

add_library(imgui STATIC "${MY_FILES}" "${MY_BACKENDS}" "${MISC_FILES}")
target_include_directories(imgui PUBLIC
    	$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>
	$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/backends>
    	$<INSTALL_INTERFACE:include>
)
