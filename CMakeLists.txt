#CMake for the Game Project LastFrame.
#Authored by Joel.
#Project Structure is split into the following:
#	Engine (static lib): Reusable code that is not specific to this game [source files].
#	Application (executable): The game application [source files].
#	Lib: External libraries used by the application and/or engine.
#	Extern: Assets used by the executable at run-time.
#	build(temporary): Temporary files generated when building executable.
#		build/buildx/bin: The executable (application), external dlls and assets (extern).
#		build/buildx/lib: The static library (engine). 
#Note: When building, files are placed in different build subfolders (buildx) corresponding to the platform they are built for.


#==========Setup==========
#Setup cmake and project properties. 
cmake_minimum_required(VERSION 3.20)
#Use filters to separate folders.
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
project("LastFrame" VERSION 1.0 LANGUAGES C CXX)

# Set the directory where custom CMake modules are stored
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

#C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#This file provides configuration info for two systems; Windows and Android
#For Windows, Engine --> static lib, Application --> executable, Lib --> external depedencies are loaded.
#For Android, both Engine and Application (including main.cpp) are loaded into a single shared library which is linked with the external dependencies used.
#==============WINDOWS==================
if(WIN32)
#Define global targets
  add_executable(application_executable)
  add_library(engine_library STATIC)
  #Add precompiled headers to speed up compilation times. 
  target_precompile_headers(engine_library PUBLIC ${CMAKE_CURRENT_LIST_DIR}/Application/Utility/pch.hpp)
  target_precompile_headers(application_executable REUSE_FROM engine_library) #Use the same pch as engine_library 
  #Preprocessor Definitions 
  target_compile_definitions(engine_library PUBLIC $<$<CONFIG:Debug>:DEBUG _DEBUG> $<$<CONFIG:Release>:NDEBUG _NDEBUG> ENGINE_ENABLE_LUA )  # <--- ADD THIS LINE 


  #Compiler Flags, use a high warning level and treat warnings as errors.
#Note: In Debug mode, ASAN and UBSAN can incur alot of performance slowdown. Only toggle in debug mode.
#Note: To enable mem leak checks, temporarily disable (remb to enable back) /fsanitize=address for the bottom target (after else())

if(NOT CODE_ANALYSIS_ON_BUILD)
target_compile_options(engine_library PUBLIC 
    # MSVC (Windows) specific warnings
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
    # GCC/Clang specific warnings
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic -Werror>
    $<$<CXX_COMPILER_ID:GNU,Clang>:$<$<CONFIG:Debug>:-fsanitize=address -fsanitize=undefined -fsanitize=leak>>
)
message("ASAN is off")
else() 
target_compile_options(engine_library PUBLIC 
    # MSVC (Windows) specific warnings
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
    # /fsanitize=address is enabled for ASAN. 
    $<$<CXX_COMPILER_ID:MSVC>:$<$<CONFIG:Debug>:/fsanitize=address>>
    # GCC/Clang specific warnings
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic -Werror>
    $<$<CXX_COMPILER_ID:GNU,Clang>:$<$<CONFIG:Debug>:-fsanitize=address -fsanitize=undefined -fsanitize=leak>>
)
endif()

#Code analysis for Visual Studio
#Enable VS code analysis for faster checks.
#Enable clang tidy code analysis for more checks.
set_target_properties(engine_library PROPERTIES
    VS_GLOBAL_RunCodeAnalysis ${CODE_ANALYSIS_ON_BUILD}
    # Do not use visual studio core guidelines if clang tidy is enabled.
    VS_GLOBAL_EnableMicrosoftCodeAnalysis true
    # Use clangtidy
    VS_GLOBAL_EnableClangTidyCodeAnalysis false
    VS_GLOBAL_ClangTidyChecks -checks=-*,clang-analyzer-*,cppcoreguidelines-*,bugprone-*,modernize-*,-modernize-use-trailing-return-type
)
set_target_properties(application_executable PROPERTIES
    VS_GLOBAL_RunCodeAnalysis ${CODE_ANALYSIS_ON_BUILD}
    # Do not use visual studio core guidelines if clang tidy is enabled.
    VS_GLOBAL_EnableMicrosoftCodeAnalysis true
    # Use clangtidy
    VS_GLOBAL_EnableClangTidyCodeAnalysis false
    VS_GLOBAL_ClangTidyChecks -checks=-*,clang-analyzer-*,cppcoreguidelines-*,bugprone-*,modernize-*,-modernize-use-trailing-return-type
)

#Output Names
set_target_properties(application_executable PROPERTIES OUTPUT_NAME LastFrame)
set_target_properties(engine_library PROPERTIES OUTPUT_NAME EngineLibrary)


# #Copies assets to build directory, every build (to refresh assets)
# #This is necessary when run from visual studio (instead of running the executable)
# add_custom_command(
#   TARGET application_executable
#   POST_BUILD
#   COMMAND ${CMAKE_COMMAND} -E copy_directory
#           ${CMAKE_SOURCE_DIR}/Extern
#           ${CMAKE_BINARY_DIR}/Extern
#   COMMENT "Copying Extern/ to build directory ${CMAKE_BINARY_DIR}/"
# )

# #Copies assets to executable directory.
# #This is necessary for executable to find it.
# add_custom_command(
#   TARGET application_executable
#   POST_BUILD
#   COMMAND ${CMAKE_COMMAND} -E copy_directory
#           ${CMAKE_SOURCE_DIR}/Extern
#           $<TARGET_FILE_DIR:application_executable>/Extern
#   COMMENT "Copying Extern/ to executable directory"
# )

#Sets the default startup project for visual studio to application_executable, other IDEs will ignore this.
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT application_executable)

#Set windows or Console Subsystem
set_target_properties(application_executable PROPERTIES
    WIN32_EXECUTABLE "$<$<CONFIG:Debug>:FALSE> $<$<CONFIG:Release>:TRUE>"
)
target_link_options(application_executable PRIVATE
        "$<$<CONFIG:Debug>:/SUBSYSTEM:CONSOLE>"
        "$<$<CONFIG:Release>:/SUBSYSTEM:WINDOWS>"
    )
#==============WINDOWS END==================

#==============ANDROID====================
elseif(ANDROID)
message("Building for Android")
  #Define global targets
  add_library(jnicpp SHARED ${CMAKE_CURRENT_LIST_DIR}/Android/app/src/main/cpp/NativeTemplate.cpp)
  
  #Add precompiled headers to speed up compilation times. 
  target_precompile_headers(jnicpp PRIVATE ${CMAKE_CURRENT_LIST_DIR}/Application/Utility/pch.hpp)
  #Preprocessor Definitions 
  target_compile_definitions(jnicpp PUBLIC $<$<CONFIG:Debug>:DEBUG _DEBUG> $<$<CONFIG:Release>:NDEBUG _NDEBUG>)


#Copies assets to android apk directory.
#This is necessary for apk to find it.
# Paths
set(EXTERN_SRC "${CMAKE_CURRENT_LIST_DIR}/Extern")
set(ASSETS_DST "${CMAKE_CURRENT_LIST_DIR}/Android/app/src/main/assets/Extern")

#So remove directory does not throw an error.
if(NOT EXISTS "${ASSETS_DST}")
    file(MAKE_DIRECTORY "${ASSETS_DST}")
endif()
# Custom target to copy assets
add_custom_target(copy_assets ALL
    COMMAND ${CMAKE_COMMAND} -E echo "Cleaning old assets..."
    COMMAND ${CMAKE_COMMAND} -E remove_directory "${ASSETS_DST}"
    COMMAND ${CMAKE_COMMAND} -E echo "Copying Extern/ to assets..."
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${EXTERN_SRC}" "${ASSETS_DST}"
    COMMENT "Copying Extern/ folder to src/main/assets"
)
add_dependencies(jnicpp copy_assets)
endif()
#==============ANDROID END==================



#==========Building of External Dependencies==========
# Include project-wide utility and dependency modules
#include(ImportDependencies)
#NOTE: importDependencies is failing and I don't know how to debug it, so Vendoring will be used instead.
#importDependencies()

# Make CMAKE_MSVC_RUNTIME_LIBRARY effective everywhere (CMake >= 3.15)
cmake_policy(SET CMP0091 NEW)

if(MSVC)
  # Pick ONE runtime for all targets. Recommended: DLL CRT (/MD, /MDd)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
  # If you truly want static CRT instead, use:
  # set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# ---- MSVC: force /MD (Release) and /MDd (Debug) for everything
cmake_policy(SET CMP0091 NEW)
if (MSVC)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif()

# ---- Jolt: do not override flags; do not use static CRT
set(OVERRIDE_CXX_FLAGS OFF CACHE BOOL "" FORCE)   # stop Jolt from rewriting CXX flags
set(JPH_USE_STATIC_VCRT OFF CACHE BOOL "" FORCE)  # OFF => /MD(/MDd), ON => /MT(/MTd)

#Build external dependencies ( Jolt lives here )
add_subdirectory(Lib)

# ---- (optional) sanity print + last-resort enforcement
if (TARGET Jolt)
  get_target_property(_rt Jolt MSVC_RUNTIME_LIBRARY)
  message(STATUS "✅ Jolt present. Runtime = ${_rt}")
  # Usually unnecessary, but safe if cache was sticky:
  set_property(TARGET Jolt PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif()

# Set variables for cmake to easily use.
set(SHARED_LIBRARIES entt glm json fmod glad Jolt stb_image xresource lua sol CACHE INTERNAL "")

#Also link important system libraries for opengl drawing
set(WINDOWS_SHARED_LIBRARIES opengl32 gdi32 user32 kernel32 glfw3 CACHE INTERNAL "")
set(WINDOWS_DEBUG_LIBRARIES imgui CACHE INTERNAL "")
set(WINDOWS_RELEASE_LIBRARIES CACHE INTERNAL "")
set(ANDROID_LIBRARIES android log GLESv3 EGL CACHE INTERNAL "")

# Disable code analysis like clang-tidy for External Libraries (for msvc, gnu. If needed, add clang here too.)
# This prevents false positives for 3rd party code.
foreach(extern_dependency IN LISTS SHARED_LIBRARIES WINDOWS_SHARED_LIBRARIES WINDOWS_DEBUG_LIBRARIES WINDOWS_RELEASE_LIBRARIES ANDROID_LIBRARIES)
  #Check if target is valid, or if it is empty/not defined.
  if(TARGET ${extern_dependency})
    #Can only set compile options on what is compiled. Avoid interface or imported libs.
    get_target_property(target_type ${extern_dependency} TYPE)
    get_target_property(is_imported ${extern_dependency} IMPORTED)
    if(NOT (is_imported) AND NOT (target_type STREQUAL "INTERFACE_LIBRARY"))
      target_compile_options(${extern_dependency} PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/analyze->
        $<$<CXX_COMPILER_ID:GNU>:-fno-analyzer>
        $<$<CXX_COMPILER_ID:Clang>:-Wno-analyzer>
      )
      message("${extern_dependency} analysis off")
    endif()

  endif()
endforeach()


#Link external libraries.
#Determine the external dependencies for this platform.
set(EXTERNAL_LIBRARY_DEPENDENCIES ${SHARED_LIBRARIES})
#Windows===============
if(WIN32)
	list(APPEND EXTERNAL_LIBRARY_DEPENDENCIES 
	${WINDOWS_SHARED_LIBRARIES} 
	$<$<CONFIG:Debug>:${WINDOWS_DEBUG_LIBRARIES}> 
	$<$<CONFIG:Release>:${WINDOWS_RELEASE_LIBRARIES}>
	)

target_link_libraries(engine_library PUBLIC ${EXTERNAL_LIBRARY_DEPENDENCIES})
target_link_libraries(application_executable PUBLIC engine_library)
target_compile_definitions(engine_library PUBLIC USE_JOLT)
#Android==============
elseif(ANDROID)
	list(APPEND EXTERNAL_LIBRARY_DEPENDENCIES ${ANDROID_LIBRARIES})
  target_link_libraries(jnicpp PUBLIC ${EXTERNAL_LIBRARY_DEPENDENCIES})
endif()


#=====Building of Engine Library and Executable, or Android Shared library======#


# ======================= AUTO PROJECT DISCOVERY =========================
#TODO: NOTE: it only works for paths without spaces.

# Get all cpp files in the engine folder and its subdirectories
file(GLOB_RECURSE ENGINE_SOURCE "Engine/*.cpp" "Engine/*.c" "Engine/*.hpp" "Engine/*.h")
# Get all cpp files in the application folder and its subdirectories
file(GLOB_RECURSE APPLICATION_SOURCE "Application/*.cpp" "Application/*.c" "Application/*.hpp" "Application/*.h")

#Set folder properties on each file so visual studio knows how to set up filters.
foreach(file IN LISTS ENGINE_SOURCE APPLICATION_SOURCE)
    #get the folder path of the file.
    file(RELATIVE_PATH relative_path "${CMAKE_SOURCE_DIR}" "${file}")
    get_filename_component(folder_path "${relative_path}" PATH)
    #Account for different filepath slash on windows.
    if(WIN32)
      string(REPLACE "/" "\\" file_path_msvc "${folder_path}")
      set(folder_path "${file_path_msvc}")
    endif()
    source_group("${folder_path}" FILES "${file}")
endforeach()

#Get the combined Include Directories of Engine and Application.
set(ENGINE_APPLICATION_INCLUDE_DIRECTORIES "Engine/" "Application/")

#Engine includes
file(GLOB_RECURSE ENGINE_HEADER_DIRECTORIES LIST_DIRECTORIES true "Engine/*")
#Only include subdirectories.
foreach(item ${ENGINE_HEADER_DIRECTORIES})
	if(IS_DIRECTORY "${item}")
		list(APPEND ENGINE_APPLICATION_INCLUDE_DIRECTORIES "${item}")
	endif()
endforeach()

#Application includes
file(GLOB_RECURSE APPLICATION_HEADER_DIRECTORIES LIST_DIRECTORIES true "Application/*")
#Only include subdirectories.
foreach(item ${APPLICATION_HEADER_DIRECTORIES})
	if(IS_DIRECTORY "${item}")
		list(APPEND ENGINE_APPLICATION_INCLUDE_DIRECTORIES "${item}")
	endif()
endforeach()


#Adding include directories and sources
if(WIN32)
  target_sources(engine_library PRIVATE ${ENGINE_SOURCE})
  target_sources(application_executable PRIVATE ${APPLICATION_SOURCE})
  #TODO: By right, the engine shouldn't need to include application headers? Need to research more on this.
  target_include_directories(engine_library PUBLIC ${ENGINE_APPLICATION_INCLUDE_DIRECTORIES})
  target_include_directories(application_executable PUBLIC ${ENGINE_APPLICATION_INCLUDE_DIRECTORIES})
elseif(ANDROID)
  target_sources(jnicpp PUBLIC ${ENGINE_SOURCE} ${APPLICATION_SOURCE})
  target_include_directories(jnicpp PUBLIC ${ENGINE_APPLICATION_INCLUDE_DIRECTORIES})
endif()


# ======================= END AUTO CONFIGURATION =========================

#Copying of dlls
#Copies fmod dll to executable path
if(WIN32)
  add_custom_command(
    TARGET application_executable
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
            $<TARGET_FILE:fmod>
            $<TARGET_FILE_DIR:application_executable>
    COMMENT "Copying fmod dll to executable directory"
  )
endif()

 