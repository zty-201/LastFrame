#Libraries.
#Configure all libraries, and let top-level cmake decide which to link.
#Put libraries in different folders depending on where they can be used.
#For example, imgui is only used in windows debug. 
#glad is used in all platforms and builds, so it is placed under shared even though it has separate versions for each platform/build.

# Shared by all platforms.
add_subdirectory(Shared/entt)
add_subdirectory(Shared/glm)
add_subdirectory(Shared/json)
add_subdirectory(Shared/fmod)
add_subdirectory(Shared/glad)
add_subdirectory(Shared/stb_image)
add_subdirectory(Shared/xresource)
add_subdirectory(Shared/lua)
add_subdirectory(Shared/sol)


# Windowsx64 Shared
add_subdirectory(Windows_x64/Shared/glfw)

# Windowsx64 Debug
add_subdirectory(Windows_x64/Debug/imgui)

# Windowsx64 Release


# Android ARM64


#Settle external interdependencies
target_link_libraries(imgui PUBLIC glfw3)
target_link_libraries(sol INTERFACE lua)