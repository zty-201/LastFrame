#Application, the game application executable

add_executable(application_executable)

#Add includes and source files to executable.
add_subdirectory(Reflection)
add_subdirectory(Utility)

#Add external libraries and Engine library, along with the include directories publicly attached to them.
target_link_libraries(application_executable EXTERNAL_LIBRARY_DEPENDENCIES engine_library)



