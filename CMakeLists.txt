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

#Setup cmake and project properties
cmake_minimum_required(VERSION 3.30)

project("LastFrame" VERSION 1.0 LANGUAGES C CXX)

# Set the directory where custom CMake modules are stored
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

#C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#Use filters to separate folders.
set_property(GLOBAL PROPERTY USE_FOLDERS ON)



#Build external dependencies.
add_subdirectory(Lib)

#Determine the external dependencies for this platform.
set(EXTERNAL_LIBRARY_DEPENDENCIES SHARED_LIBRARIES)
#Windows
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
	list(APPEND EXTERNAL_LIBRARY_DEPENDENCIES $<$<CONFIG:Debug>:WINDOWS_DEBUG_LIBRARIES> $<$<CONFIG:Release>:WINDOWS_RELEASE_LIBRARIES>)
endif()
#Android
if(CMAKE_SYSTEM_NAME STREQUAL "Android")
	list(APPEND EXTERNAL_LIBRARY_DEPENDENCIES ANDROID_LIBRARIES)
endif()
	

#Build Engine as static lib
add_subdirectory(Engine)

#Build Application as executable
add_subdirectory(Application)

#Add precompiled headers to speed up compilation times
target_precompile_headers(engine_library PRIVATE ${CMAKE_CURRENT_LIST_DIR}/Application/Utility/pch.hpp)
target_precompile_headers(application_executable PRIVATE ${CMAKE_CURRENT_LIST_DIR}/Application/Utility/pch.hpp)


#Preprocessor Definitions 
target_compile_definitions(engine_library PRIVATE $<$<CONFIG:Debug>:DEBUG> $<$<CONFIG:Release>:NDEBUG>)
target_compile_definitions(application_executable PRIVATE $<$<CONFIG:Debug>:DEBUG> $<$<CONFIG:Release>:NDEBUG>)
#Compiler Flags, use a high warning level and treat warnings as errors.
target_compile_definitions(engine_library PRIVATE 
    # MSVC (Windows) specific warnings
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
    # GCC/Clang specific warnings
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic -Werror>
)
target_compile_definitions(application_executable PRIVATE 
    # MSVC (Windows) specific warnings
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
    # GCC/Clang specific warnings
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic -Werror>
)


