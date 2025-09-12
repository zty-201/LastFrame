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
#Setup cmake and project properties
cmake_minimum_required(VERSION 3.25)
#Use filters to separate folders.
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
project("LastFrame" VERSION 1.0 LANGUAGES C CXX)

# Set the directory where custom CMake modules are stored
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

#C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#Define global targets
add_executable(application_executable)
add_library(engine_library STATIC)



#Add precompiled headers to speed up compilation times
target_precompile_headers(engine_library PRIVATE ${CMAKE_CURRENT_LIST_DIR}/Application/Utility/pch.hpp)
target_precompile_headers(application_executable PRIVATE ${CMAKE_CURRENT_LIST_DIR}/Application/Utility/pch.hpp)


#Preprocessor Definitions 
target_compile_definitions(engine_library PUBLIC $<$<CONFIG:Debug>:DEBUG> $<$<CONFIG:Release>:NDEBUG>)
target_compile_definitions(application_executable PUBLIC $<$<CONFIG:Debug>:DEBUG> $<$<CONFIG:Release>:NDEBUG>)


#Compiler Flags, use a high warning level and treat warnings as errors.
target_compile_options(engine_library PUBLIC 
    # MSVC (Windows) specific warnings
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
    # GCC/Clang specific warnings
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic -Werror>
)
target_compile_options(application_executable PUBLIC 
    # MSVC (Windows) specific warnings
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
    # GCC/Clang specific warnings
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic -Werror>
)

#Output Names
set_target_properties(application_executable PROPERTIES OUTPUT_NAME LastFrame)
set_target_properties(engine_library PROPERTIES OUTPUT_NAME EngineLibrary)

# #Output paths
# set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/${CMAKE_SYSTEM_NAME}/$<CONFIG>")
# set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/${CMAKE_SYSTEM_NAME}/$<CONFIG>")
# set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/${CMAKE_SYSTEM_NAME}/$<CONFIG>")


#Copies assets to build directory, every build (to refresh assets)
#This is necessary when run from visual studio (instead of running the executable)
add_custom_command(
  TARGET application_executable
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
          ${CMAKE_SOURCE_DIR}/Extern
          ${CMAKE_BINARY_DIR}/Extern
  COMMENT "Copying Extern/ to build directory ${CMAKE_BINARY_DIR}/"
)

#Copies assets to executable directory.
#This is necessary for executable to find it.
add_custom_command(
  TARGET application_executable
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
          ${CMAKE_SOURCE_DIR}/Extern
          $<TARGET_FILE_DIR:application_executable>/Extern
  COMMENT "Copying Extern/ to executable directory"
)

#Sets the default startup project for visual studio to application_executable, other IDEs will ignore this.
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT application_executable)


#==========Building of External Dependencies==========
# Include project-wide utility and dependency modules
include(ImportDependencies)
#NOTE: importDependencies is failing and I don't know how to debug it, so Vendoring will be used instead.
#importDependencies()

#Build external dependencies.
add_subdirectory(Lib)

# Set variables for cmake to easily use.
set(SHARED_LIBRARIES entt glm json CACHE INTERNAL "")
#Also link important system libraries for opengl drawing
set(WINDOWS_SHARED_LIBRARIES opengl32 gdi32 user32 kernel32 glfw3 CACHE INTERNAL "")
set(WINDOWS_DEBUG_LIBRARIES glad_debug imgui CACHE INTERNAL "")
set(WINDOWS_RELEASE_LIBRARIES glad_release CACHE INTERNAL "")
set(ANDROID_LIBRARIES "" CACHE INTERNAL "")


#Determine the external dependencies for this platform.
set(EXTERNAL_LIBRARY_DEPENDENCIES ${SHARED_LIBRARIES})
#Windows
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
	list(APPEND EXTERNAL_LIBRARY_DEPENDENCIES 
	${WINDOWS_SHARED_LIBRARIES} 
	$<$<CONFIG:Debug>:${WINDOWS_DEBUG_LIBRARIES}> 
	$<$<CONFIG:Release>:${WINDOWS_RELEASE_LIBRARIES}>
	)
endif()
#Android
if(CMAKE_SYSTEM_NAME STREQUAL "Android")
	list(APPEND EXTERNAL_LIBRARY_DEPENDENCIES ${ANDROID_LIBRARIES})
endif()

#Linking Libraries
target_link_libraries(engine_library PUBLIC ${EXTERNAL_LIBRARY_DEPENDENCIES})
target_link_libraries(application_executable PUBLIC engine_library)

#=====Building of Engine Library and Executable======#

#Call cmake on subdirectories
# add_subdirectory(Engine)
# add_subdirectory(Application)

# ======================= AUTO PROJECT DISCOVERY =========================
#TODO: NOTE: it only works for paths without spaces.

# Get all cpp files in the engine folder and its subdirectories
file(GLOB_RECURSE ENGINE_SOURCE "Engine/*.cpp" "Engine/*.c" "Engine/*.hpp" "Engine/*.h")
target_sources(engine_library PUBLIC ${ENGINE_SOURCE})

# Get all cpp files in the application folder and its subdirectories
file(GLOB_RECURSE APPLICATION_SOURCE "Application/*.cpp" "Application/*.c" "Application/*.hpp" "Application/*.h")
target_sources(application_executable PUBLIC ${APPLICATION_SOURCE})

#Set folder properties on each file so visual studio knows how to set up filters.
foreach(file IN LISTS ENGINE_SOURCE APPLICATION_SOURCE)
    #get the folder path of the file.
    file(RELATIVE_PATH relative_path "${CMAKE_SOURCE_DIR}" "${file}")
    get_filename_component(folder_path "${relative_path}" PATH)
    #Account for different filepath slash on windows.
    if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
      string(REPLACE "/" "\\" file_path_msvc "${folder_path}")
      set(folder_path "${file_path_msvc}")
    endif()
    source_group("${folder_path}" FILES "${file}")
endforeach()

#Combined Include Directories of Engine and Application.
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

#TODO: By right, the engine shouldn't need to include application headers? Need to research more on this.
target_include_directories(engine_library PUBLIC ${ENGINE_APPLICATION_INCLUDE_DIRECTORIES})
target_include_directories(application_executable PUBLIC ${ENGINE_APPLICATION_INCLUDE_DIRECTORIES})

# ======================= END AUTO CONFIGURATION =========================
