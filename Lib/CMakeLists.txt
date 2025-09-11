#Libraries.
#Build all libraries, and let top-level cmake decide which to link.

# Shared by all platforms.
#add_subdirectory(Shared/entt)
#add_subdirectory(Shared/glm)
#add_subdirectory(Shared/json)

# Windowsx64 Shared
#add_subdirectory(Windows_x64/Shared/glfw)

# Windowsx64 Debug
add_subdirectory(Windows_x64/Debug/glad_debug)
#add_subdirectory(Windows_x64/Debug/imgui)

# Windowsx64 Release
add_subdirectory(Windows_x64/Release/glad_release)

# Android ARM64


#Settle external interdependencies
#target_include_directories(imgui PUBLIC "${GLFW_INCLUDE_DIRECTORY}")
#target_link_libraries(imgui PUBLIC glfw3)