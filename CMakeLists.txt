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

#Toggle for console vs windows app (on desktop).
option(BUILD_GUI_APP "Build as a GUI application (no console window)" OFF)


#Setup cmake and project properties
cmake_minimum_required(VERSION 3.25)

project("LastFrame" VERSION 1.0 LANGUAGES C CXX)

# Set the directory where custom CMake modules are stored
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

#Set output directories
set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_LIST_DIR}/bin/${CMAKE_SYSTEM_NAME}/${CONFIG}")
set(LIBRARY_OUTPUT_PATH "${CMAKE_CURRENT_LIST_DIR}/bin/${CMAKE_SYSTEM_NAME}/${CONFIG}/lib")

#Copying assets to executable folder
file(COPY "${CMAKE_CURRENT_LIST_DIR}/Extern" DESTINATION "${CMAKE_CURRENT_LIST_DIR}/bin/${CMAKE_SYSTEM_NAME}/${Configuration}")

#C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#Use filters to separate folders.
set_property(GLOBAL PROPERTY USE_FOLDERS ON)


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
message("External Libs: ${EXTERNAL_LIBRARY_DEPENDENCIES}")

#Build Engine as static lib
add_subdirectory(Engine)
target_link_libraries(engine_library ${EXTERNAL_LIBRARY_DEPENDENCIES})

#Build Application as executable
add_subdirectory(Application)
target_link_libraries(application_executable PUBLIC engine_library)


# Adding glfw.lib library path to both executable and library, as glfw target was unable to link in the previous step.
#TODO: Temporary(?) solution.
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
	target_link_directories(engine_library PUBLIC ${GLFW_BINARY}/)
	target_link_directories(imgui PRIVATE ${GLFW_BINARY}/)

	target_include_directories(engine_library PUBLIC ${GLFW_INCLUDE_DIRECTORY})
	target_include_directories(imgui PRIVATE ${GLFW_INCLUDE_DIRECTORY})
endif()





#Add precompiled headers to speed up compilation times
target_precompile_headers(engine_library PRIVATE ${CMAKE_CURRENT_LIST_DIR}/Application/Utility/pch.hpp)
target_precompile_headers(application_executable PRIVATE ${CMAKE_CURRENT_LIST_DIR}/Application/Utility/pch.hpp)


#Preprocessor Definitions 
target_compile_definitions(engine_library PRIVATE $<$<CONFIG:Debug>:DEBUG> $<$<CONFIG:Release>:NDEBUG>)
target_compile_definitions(application_executable PRIVATE $<$<CONFIG:Debug>:DEBUG> $<$<CONFIG:Release>:NDEBUG>)



#Compiler Flags, use a high warning level and treat warnings as errors.
target_compile_definitions(engine_library PRIVATE 
    # MSVC (Windows) specific warnings
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX /0d /Zi>
    # GCC/Clang specific warnings
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic -Werror>
)
target_compile_definitions(application_executable PRIVATE 
    # MSVC (Windows) specific warnings
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX /02>
    # GCC/Clang specific warnings
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic -Werror>
)

#Output Names
set_target_properties(application_executable PROPERTIES OUTPUT_NAME LastFrame)
set_target_properties(engine_library PROPERTIES OUTPUT_NAME EngineLibrary)



