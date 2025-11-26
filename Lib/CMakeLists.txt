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
add_subdirectory(Shared/OpenFBX)

# --- Assimp ---
# Turn off parts we don't need
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "Disable Assimp tests" FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "Disable Assimp tools" FORCE)
set(ASSIMP_INSTALL OFF CACHE BOOL "Don't create install targets" FORCE)
set(ASSIMP_WARNINGS_AS_ERRORS OFF CACHE BOOL "Disable warnings as errors for assimp" FORCE)
set(ASSIMP_NO_EXPORT ON CACHE BOOL "Disable Assimp's exporting feature" FORCE)
set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF CACHE BOOL "Disable all importers by default" FORCE)
set(ASSIMP_BUILD_OBJ_IMPORTER ON CACHE BOOL "Enable ONLY the OBJ importer" FORCE)

# Force static library build
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build Assimp as a static library" FORCE)

if(ANDROID)

    set(ASSIMP_BUILD_ZLIB ON CACHE BOOL "Build zlib for Android" FORCE)
    
    # Disable Android-problematic features
    set(ASSIMP_ANDROID_JNIIOSYSTEM OFF CACHE BOOL "Disable Android JNI IO (not needed)" FORCE)
    set(ASSIMP_BUILD_MINIZIP OFF CACHE BOOL "Don't build minizip on Android" FORCE)
    set(ASSIMP_BUILD_DRACO OFF CACHE BOOL "Don't build Draco on Android" FORCE)
endif()

add_subdirectory(Shared/assimp)

# DEBUG (temporary):
if(ANDROID)
    get_directory_property(assimp_targets DIRECTORY Shared/assimp BUILDSYSTEM_TARGETS)
    message(STATUS "=== ASSIMP TARGETS ===")
    foreach(target IN LISTS assimp_targets)
        message(STATUS "  ${target}")
    endforeach()
endif()

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