#Libraries.
#Build all libraries, and let top-level cmake decide which to link.

# Shared by all platforms.
add_subdirectory(Shared/entt)
add_subdirectory(Shared/glm)
add_subdirectory(Shared/json)

# Windowsx64 Shared
add_subdirectory(Windows_x64/Shared/glfw)

# Windowsx64 Debug
add_subdirectory(Windows_x64/Debug/glad_debug)
add_subdirectory(Windows_x64/Debug/imgui)

# Windowsx64 Release
add_subdirectory(Windows_x64/Release/glad_release)

# Android ARM64


# Set variables for top-level cmake to easily use.
set(SHARED_LIBRARIES entt glm json CACHE INTERNAL "")
set(WINDOWS_SHARED_LIBRARIES glfw CACHE INTERNAL "")
set(WINDOWS_DEBUG_LIBRARIES glad_debug imgui CACHE INTERNAL "")
set(WINDOWS_RELEASE_LIBRARIES glad_release CACHE INTERNAL "")
set(ANDROID_LIBRARIES "" CACHE INTERNAL "")