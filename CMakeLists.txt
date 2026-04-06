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
if(WIN32)
  set(CMAKE_VS_USE_POWERSHELL 0)
  if(MSVC)
    # Add /FS to prevent PDB access conflicts during parallel builds
    add_compile_options(/FS)
  endif()
endif()
# Enable IPO (LTO) exclusively for Release configurations globally
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
# -------------------------------------------------------------------------------------
# Global compile setting: align all Debug iterator levels with Jolt (Release-like)
# This ensures the precompiled header (cmake_pch.obj) and all TUs agree with Jolt.lib.
# -------------------------------------------------------------------------------------
if(WIN32)
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /D_ITERATOR_DEBUG_LEVEL=0")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /D_ITERATOR_DEBUG_LEVEL=0")
endif()

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
  #target_precompile_headers(application_executable REUSE_FROM engine_library) #Use the same pch as engine_library 
  #Preprocessor Definitions 
  target_compile_definitions(engine_library PUBLIC $<$<CONFIG:Debug>:DEBUG _DEBUG> $<$<CONFIG:Release>:NDEBUG _NDEBUG> ENGINE_ENABLE_LUA )  # <--- ADD THIS LINE 


  # Ensure the default ImGui dock layout exists in the build tree
  set(IMGUI_DEFAULT_LAYOUT_SRC "${CMAKE_SOURCE_DIR}/Extern/LevelEditor/imgui_default.ini")
  set(IMGUI_DEFAULT_LAYOUT_BIN "${CMAKE_BINARY_DIR}/imgui.ini")
  configure_file(${IMGUI_DEFAULT_LAYOUT_SRC} ${IMGUI_DEFAULT_LAYOUT_BIN} COPYONLY)

  #Compiler Flags, use a high warning level and treat warnings as errors.
#Note: In Debug mode, ASAN and UBSAN can incur alot of performance slowdown. Only toggle in debug mode.
#Note: To enable mem leak checks, temporarily disable (remb to enable back) /fsanitize=address for the bottom target (after else())

if(NOT CODE_ANALYSIS_ON_BUILD)
target_compile_options(engine_library PUBLIC 
# MSVC (Windows) specific warnings
	$<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
	# === MSVC Release Optimizations ===
	# /O2 (Max speed), /Oi (Intrinsics), /Ot (Favor fast code), /fp:fast (Fast math), /Zc:inline (Remove unreferenced COMDATs), /Gw (Optimize Global Data)
	$<$<AND:$<CONFIG:Release>,$<CXX_COMPILER_ID:MSVC>>:/O2 /Oi /Ot /Zc:inline /Gw>
	
	
	# GCC/Clang specific warnings
	$<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic -Werror>
	
	# === GCC/Clang Release Optimizations ===
	$<$<AND:$<CONFIG:Release>,$<CXX_COMPILER_ID:GNU,Clang>>:-O3>
	
	$<$<CXX_COMPILER_ID:GNU,Clang>:$<$<CONFIG:Debug>:-fsanitize=address -fsanitize=undefined -fsanitize=leak>>
)
message("ASAN is off")
else() 
target_compile_options(engine_library PUBLIC 
# MSVC (Windows) specific warnings
	$<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
	
	# === MSVC Release Optimizations ===
	# /O2 (Max speed), /Oi (Intrinsics), /Ot (Favor fast code), /fp:fast (Fast math), /Zc:inline (Remove unreferenced COMDATs), /Gw (Optimize Global Data)
	$<$<AND:$<CONFIG:Release>,$<CXX_COMPILER_ID:MSVC>>:/O2 /Oi /Ot /Zc:inline /Gw>
	
	# /fsanitize=address is enabled for ASAN 
	$<$<CXX_COMPILER_ID:MSVC>:$<$<CONFIG:Debug>:/fsanitize=address>>
	
	# GCC/Clang specific warnings
	$<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic -Werror>
	
	# === GCC/Clang Release Optimizations ===
	$<$<AND:$<CONFIG:Release>,$<CXX_COMPILER_ID:GNU,Clang>>:-O3>
	
	$<$<CXX_COMPILER_ID:GNU,Clang>:$<$<CONFIG:Debug>:-fsanitize=address -fsanitize=undefined -fsanitize=leak>>
)
endif()

#Code analysis for Visual Studio
#Enable VS code analysis for faster checks.
#Enable clang tidy code analysis for more checks.
# NOTE: Enabling VS_GLOBAL_RunCodeAnalysis causes precompiled headers for application_executable to fail (as the app reuses pch from engine_library)
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

#Sets the default startup project for visual studio to application_executable, other IDEs will ignore this.
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT application_executable)

#Set windows or Console Subsystem
set_target_properties(application_executable PROPERTIES
	WIN32_EXECUTABLE "$<$<CONFIG:Debug>:FALSE> $<$<CONFIG:Release>:FALSE>"
)
target_link_options(application_executable PRIVATE
		"$<$<CONFIG:Debug>:/SUBSYSTEM:CONSOLE>"
		"$<$<CONFIG:Release>:/SUBSYSTEM:CONSOLE>"

    # === MSVC Linker Release Optimizations ===
		# /OPT:REF (Eliminates functions/data never referenced), /OPT:ICF (Folds identical functions to save cache/space), /INCREMENTAL:NO (Required for maximum release optimization)
		$<$<AND:$<CONFIG:Release>,$<CXX_COMPILER_ID:MSVC>>:/OPT:REF /OPT:ICF /INCREMENTAL:NO>
	)
#==============WINDOWS END==================

#==============ANDROID====================
elseif(ANDROID)
message("Building for Android")
  #Define global targets
  add_library(jnicpp SHARED ${CMAKE_CURRENT_LIST_DIR}/Android/app/src/main/cpp/NativeTemplate.cpp)
  
  #Add precompiled headers to speed up compilation times. 
  target_precompile_headers(jnicpp PUBLIC ${CMAKE_CURRENT_LIST_DIR}/Application/Utility/pch.hpp)
  #Preprocessor Definitions 
  target_compile_definitions(jnicpp PUBLIC $<$<CONFIG:Debug>:DEBUG _DEBUG> $<$<CONFIG:Release>:NDEBUG _NDEBUG> ENGINE_ENABLE_LUA)
# Android (Clang) Compiler Optimizations
  target_compile_options(jnicpp PUBLIC 
      $<$<CONFIG:Release>:-O3 -ftree-vectorize>
  )
  target_link_options(jnicpp PRIVATE 
      $<$<CONFIG:Release>:-Wl,--gc-sections,-O2> # Linker garbage collection
  )

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
	COMMAND ${CMAKE_COMMAND} -E rm -rf "${ASSETS_DST}"
	COMMAND ${CMAKE_COMMAND} -E echo "Copying Extern/ to assets..."
	COMMAND ${CMAKE_COMMAND} -E copy_directory "${EXTERN_SRC}" "${ASSETS_DST}"
	COMMENT "Copying Extern/ folder to src/main/assets"
)
add_dependencies(jnicpp copy_assets)
endif()
#==============ANDROID END==================


#==========Building of External Dependencies==========
# Include project-wide utility and dependency modules
include(ImportDependencies)
importDependencies()

# -----------------------------------------------------------------------------
# Jolt overrides (must be set **before** add_subdirectory(Lib))
# Keep Jolt aligned with our toolchain and avoid CRT/iterator mismatches.
# -----------------------------------------------------------------------------

# Ensure we use dynamic CRT (/MD, /MDd) everywhere
cmake_policy(SET CMP0091 NEW)
if(MSVC)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif()

# Tell Jolt explicitly to use dynamic CRT too (matching the line we fixed above)
set(USE_STATIC_MSVC_RUNTIME_LIBRARY OFF CACHE BOOL "" FORCE)

# Make sure Debug builds don’t enable the MSVC “checked iterators” level that conflicts with Jolt
if(WIN32)
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /D_ITERATOR_DEBUG_LEVEL=0")
endif()

# Optional Jolt feature toggles (safe defaults for your engine)
# Turn off exceptions/RTTI for smaller, faster Jolt (unless your code needs them)
set(CPP_EXCEPTIONS_ENABLED OFF CACHE BOOL "" FORCE)
set(CPP_RTTI_ENABLED OFF CACHE BOOL "" FORCE)

# Keep platform CPU feature requirements conservative
# (You can bump these if you’re targeting only your dev machine.)
set(USE_SSE4_1 ON  CACHE BOOL "" FORCE)
set(USE_SSE4_2 ON  CACHE BOOL "" FORCE)
set(USE_AVX     OFF CACHE BOOL "" FORCE)
set(USE_AVX2    OFF CACHE BOOL "" FORCE)
set(USE_AVX512  OFF CACHE BOOL "" FORCE)
set(USE_LZCNT   ON  CACHE BOOL "" FORCE)
set(USE_TZCNT   ON  CACHE BOOL "" FORCE)
set(USE_F16C    ON  CACHE BOOL "" FORCE)
set(USE_FMADD   ON  CACHE BOOL "" FORCE)

# Leave IPO/LTO off unless you plan to enable it for **your** targets too
# (mismatched LTO between app/lib can cause linker issues or lost benefits).
set(INTERPROCEDURAL_OPTIMIZATION OFF CACHE BOOL "" FORCE)

#Build external dependencies ( Jolt lives here )
add_subdirectory(Lib)

# Safety: enforce CRT on Jolt target
if (TARGET Jolt)
  set_property(TARGET Jolt PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

  # Manually force LTO *only* for the Release configuration on Jolt
  set_property(TARGET Jolt PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
endif()
#---------------------------------------------END JOLT CONFIG----------------------------------------------------------------

# Set variables for cmake to easily use.
set(SHARED_LIBRARIES entt glm json fmod glad Jolt stb_image xresource lua sol assimp openfbx gli CACHE INTERNAL "")

#Also link important system libraries for opengl drawing
set(WINDOWS_SHARED_LIBRARIES
    opengl32 gdi32 user32 kernel32 glfw3 compressonator
    mfplat mfreadwrite mfuuid propsys ole32
    CACHE INTERNAL "")
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
		$<$<CXX_COMPILER_ID:Clang>:-w>
	  )
	  message("${extern_dependency} analysis off")
	else()
	set_source_files_properties(${extern_dependency} PROPERTIES CXX_CLANG_TIDY "" COMPILE_OPTIONS "$<$<CXX_COMPILER_ID:MSVC>:/analyze->")
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
    
    # DEBUG: Print all libraries
    message(STATUS "=== ANDROID LIBRARIES ===")
    foreach(lib IN LISTS EXTERNAL_LIBRARY_DEPENDENCIES)
        message(STATUS "  Linking: ${lib}")
    endforeach()
    
    target_link_libraries(jnicpp PUBLIC ${EXTERNAL_LIBRARY_DEPENDENCIES})
endif()

#=====Building of Engine Library and Executable, or Android Shared library======#



# ======================= AUTO PROJECT DISCOVERY =========================
#TODO: NOTE: it only works for paths without spaces.

# Get all cpp files in the engine, application folder and its subdirectories (including main.cpp)
file(GLOB_RECURSE ENGINE_SOURCE "Engine/*.cpp" "Engine/*.c" "Engine/*.hpp" "Engine/*.h" "Application/*.cpp" "Application/*.c" "Application/*.hpp" "Application/*.h")
list(REMOVE_ITEM ENGINE_SOURCE "Application/win_main.cpp") #Exclude files belonging to application_executable
list(APPEND ENGINE_SOURCE
    ${CMAKE_CURRENT_LIST_DIR}/Engine/Graphics/VideoClip.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Engine/Graphics/VideoClip.h
)
# Get only the main file for windows.
file(GLOB APPLICATION_SOURCE "Application/win_main.cpp")
# Get all cpp/hpp testing files
file(GLOB_RECURSE TESTER_SOURCE "Tester/*.cpp" "Tester/*.c" "Tester/*.hpp" "Tester/*.h")

#Set folder properties on each file so visual studio knows how to set up filters.
foreach(file IN LISTS ENGINE_SOURCE APPLICATION_SOURCE TESTER_SOURCE)
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

set(TESTER_INCLUDE_DIRECTORIES "Tester/")

#Engine includes
file(GLOB_RECURSE ENGINE_HEADER_DIRECTORIES LIST_DIRECTORIES true "Engine/*")
#Only include subdirectories, not files.
foreach(item ${ENGINE_HEADER_DIRECTORIES})
	if(IS_DIRECTORY "${item}")
		list(APPEND ENGINE_APPLICATION_INCLUDE_DIRECTORIES "${item}")
	endif()
endforeach()

#Application includes
file(GLOB_RECURSE APPLICATION_HEADER_DIRECTORIES LIST_DIRECTORIES true "Application/*")
#Only include subdirectories, not files.
foreach(item ${APPLICATION_HEADER_DIRECTORIES})
	if(IS_DIRECTORY "${item}")
		list(APPEND ENGINE_APPLICATION_INCLUDE_DIRECTORIES "${item}")
	endif()
endforeach()

#Tester includes
file(GLOB_RECURSE TESTER_INCLUDE_DIRECTORIES LIST_DIRECTORIES true "Tester/*")
#Only include subdirectories, not files.
foreach(item ${TESTER_INCLUDE_DIRECTORIES})
	if(IS_DIRECTORY "${item}")
		list(APPEND TESTER_INCLUDE_DIRECTORIES "${item}")
	endif()
endforeach()



#Adding include directories and sources
if(WIN32)
  target_sources(engine_library PRIVATE ${ENGINE_SOURCE})
  target_sources(application_executable PRIVATE ${APPLICATION_SOURCE} )

  #TODO: By right, the engine shouldn't need to include application headers? Need to research more on this.
  #TODO2: Engine lib now consists of folders and files in both Engine and Application, except for Application/win_main.cpp. This is because of mistaken configuration from last time that's hard to fix now.
  target_include_directories(engine_library PUBLIC ${ENGINE_APPLICATION_INCLUDE_DIRECTORIES})
  target_include_directories(application_executable PUBLIC ${ENGINE_APPLICATION_INCLUDE_DIRECTORIES})
elseif(ANDROID)
  target_sources(jnicpp PUBLIC ${ENGINE_SOURCE} ${APPLICATION_SOURCE})
  target_include_directories(jnicpp PUBLIC ${ENGINE_APPLICATION_INCLUDE_DIRECTORIES})
  target_include_directories(jnicpp PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Extern/glm")
endif()

#Adding /bigobj for Script.cpp as it's too big
if (MSVC)
    set_source_files_properties(
        Engine/Scripting/Script.cpp
        Application/Reflection/ReflectionRegister.cpp
        PROPERTIES COMPILE_OPTIONS "/bigobj"
    )
endif()


# ======================= END AUTO CONFIGURATION =========================


#====================Creation of Tester Project=======================
#Tester is only used for CI Pipeline to run unit/integration tests before pushing to git remote.
#As such, it should not be built together with applicationexecutable for typical development unless manually specified.
#Do not include it when shipping.
if(WIN32) #Disabled for android, only testing for Android is whether it can build. Open to change if android needs to be tested further.
enable_testing()

add_executable(tester_executable)
target_sources(tester_executable PRIVATE ${TESTER_SOURCE} )
target_include_directories(tester_executable PUBLIC ${ENGINE_APPLICATION_INCLUDE_DIRECTORIES} ${TESTER_INCLUDE_DIRECTORIES})
target_link_libraries(tester_executable PRIVATE engine_library GTest::gtest)

include(GoogleTest)
gtest_discover_tests(tester_executable CONFIGURATIONS Debug Release)
endif()


#======================================================================

#Copying of dlls
#Copies fmod dll to executable path
if(WIN32)
  foreach(EXEC_TARGET application_executable tester_executable)
    add_custom_command(
      TARGET ${EXEC_TARGET}
      PRE_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
              ${IMGUI_DEFAULT_LAYOUT_BIN}
              $<TARGET_FILE_DIR:${EXEC_TARGET}>/imgui.ini
      COMMENT "Copying default ImGui layout to executable directory"
    )
    add_custom_command(
      TARGET ${EXEC_TARGET}
      POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy
              $<TARGET_FILE:fmod>
              $<TARGET_FILE_DIR:${EXEC_TARGET}>
      COMMENT "Copying fmod dll to executable directory"
    )
    add_custom_command(
      TARGET ${EXEC_TARGET}
      POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
              $<TARGET_FILE:compressonator>
              $<TARGET_FILE_DIR:${EXEC_TARGET}>
      COMMENT "Copying Compressonator dll to executable directory"
    )
  endforeach()
endif()

 
