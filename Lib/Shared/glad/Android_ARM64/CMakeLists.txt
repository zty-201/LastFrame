# glad for android
# Load only if android.

if(ANDROID)
	add_library(glad SHARED ${CMAKE_CURRENT_LIST_DIR}/src/glad.c)
	target_include_directories(glad PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)
endif()