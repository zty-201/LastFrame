include(FetchContent)

# Macro to import GLFW
macro(import_glfw)
    if(NOT TARGET glfw)
        FetchContent_Declare(
            glfw
            GIT_REPOSITORY https://github.com/glfw/glfw.git
            GIT_TAG 3.3.10
        )
        set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
        set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
        set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(glfw)
    endif()
endmacro()

# Macro to import glm
macro(import_glm)
    if(NOT TARGET glm)
        FetchContent_Declare(
            glm
            GIT_REPOSITORY https://github.com/g-truc/glm.git
            GIT_TAG master
        )
        FetchContent_MakeAvailable(glm)
    endif()
endmacro()

# Macro to import ImGui
macro(import_imgui)
    if(NOT TARGET imgui)
        FetchContent_Declare(
            imgui
            GIT_REPOSITORY https://github.com/ocornut/imgui.git
            GIT_TAG v1.90.4
        )
        FetchContent_MakeAvailable(imgui)
        
        # NOTE: The original file had a manual library creation for ImGui.
        # However, with FetchContent_MakeAvailable, the ImGui library is
        # automatically available. You may no longer need this manual creation.
        # If the library still needs to be created, you'll have to manually
        # fetch and add it. Given the context, FetchContent_MakeAvailable
        # is the desired change. The following code is based on the assumption
        # that ImGui does not provide a CMakeLists.txt and must be manually built.
        # If ImGui has its own CMakeLists.txt now, this part would be different.
        
        # The following manual library creation block is kept from the original
        # to ensure the ImGui library is properly built and linked, as
        # FetchContent_MakeAvailable alone won't work if the project doesn't
        # provide a CMakeLists.txt.
        
        # Manual ImGui library creation and configuration
        set(IMGUI_SOURCES
            ${imgui_SOURCE_DIR}/imgui.cpp
            ${imgui_SOURCE_DIR}/imgui_demo.cpp
            ${imgui_SOURCE_DIR}/imgui_draw.cpp
            ${imgui_SOURCE_DIR}/imgui_tables.cpp
            ${imgui_SOURCE_DIR}/imgui_widgets.cpp
            ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
            ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
        )

        add_library(imgui STATIC ${IMGUI_SOURCES})
        target_include_directories(imgui PUBLIC 
            ${imgui_SOURCE_DIR}
            ${imgui_SOURCE_DIR}/backends
        )
        target_link_libraries(imgui PUBLIC glfw libglew_static)
        
        # Set C++ standard for ImGui
        set_property(TARGET imgui PROPERTY CXX_STANDARD 11)
        set_property(TARGET imgui PROPERTY CXX_STANDARD_REQUIRED ON)
    endif()
endmacro()

# Macro to import EnTT
macro(import_entt)
    if(NOT TARGET entt)
        FetchContent_Declare(
            entt
            GIT_REPOSITORY https://github.com/skypjack/entt.git
            GIT_TAG v3.13.0
        )
        FetchContent_MakeAvailable(entt)
    endif()
endmacro()

# Macro to import nlohmann/json
macro(import_nlohmann_json)
    if(NOT TARGET nlohmann_json::nlohmann_json)
        FetchContent_Declare(
            nlohmann_json
            GIT_REPOSITORY https://github.com/nlohmann/json.git
            GIT_TAG v3.11.3
        )
        FetchContent_MakeAvailable(nlohmann_json)
    endif()
endmacro()

# Macro to import glad
macro(import_glad)
    if(NOT TARGET glad)
        FetchContent_Declare(
            glad
            GIT_REPOSITORY https://github.com/Dav1dde/glad.git
            GIT_TAG v2.0.6
        )
        FetchContent_MakeAvailable(glad)
    endif()
endmacro()

# Macro to import Jolt Physics
macro(import_jolt)
    if(NOT TARGET Jolt)
        # Make sure Jolt uses /MD(/MDd) and doesn't override flags
        set(OVERRIDE_CXX_FLAGS OFF CACHE BOOL "" FORCE)
        set(JPH_USE_STATIC_VCRT OFF CACHE BOOL "" FORCE)

        # (optional, but nice for MSVC debug)
        set(GENERATE_DEBUG_SYMBOLS ON CACHE BOOL "" FORCE)
        set(JPH_DEBUG_SYMBOL_FORMAT "" CACHE STRING "")

        # You can pin a release tag or a commit
        FetchContent_Declare(
            Jolt
            GIT_REPOSITORY https://github.com/jrouwe/JoltPhysics.git
            GIT_TAG v5.3.1
        )
        FetchContent_MakeAvailable(Jolt)  # provides static target "Jolt"
    endif()
endmacro()

# Macro to import all dependencies
macro(importDependencies)
    message(STATUS "Starting to import dependencies...")
    message(STATUS "Importing GLFW...")
    import_glfw()
    message(STATUS "GLFW imported successfully.")
    message(STATUS "Importing Glad...")
    import_glad()
    message(STATUS "Glad imported successfully.")
    message(STATUS "Importing GLM...")
    import_glm()
    message(STATUS "GLM imported successfully.")
    message(STATUS "Importing ImGui...")
    import_imgui()
    message(STATUS "ImGui imported successfully.")
    message(STATUS "Importing EnTT...")
    import_entt()
    message(STATUS "EnTT imported successfully.")
    message(STATUS "Importing nlohmann/json...")
    import_nlohmann_json()
    message(STATUS "nlohmann/json imported successfully.")
    import_jolt()
    message(STATUS "Jolt imported successfully.")
    message(STATUS "All dependencies have been imported successfully.")
endmacro()