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

# --- Assimp ---
# Turn off parts we don't need
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "Disable Assimp tests" FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "Disable Assimp tools" FORCE)
set(ASSIMP_INSTALL OFF CACHE BOOL "Don't create install targets" FORCE)

# === THIS IS THE FIX ===
# Force Assimp to build as a static library (.lib) instead of a shared one (.dll)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build Assimp as a static library" FORCE)
add_subdirectory(Shared/assimp)
add_subdirectory(Shared/JoltPhysics/Build)

# Windowsx64 Shared
add_subdirectory(Windows_x64/Shared/glfw)

# Windowsx64 Debug
add_subdirectory(Windows_x64/Debug/imgui)

# Windowsx64 Release


# Android ARM64


#Settle external interdependencies
target_link_libraries(imgui PUBLIC glfw3)
target_link_libraries(sol INTERFACE lua)