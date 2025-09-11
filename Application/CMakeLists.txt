#Application, the game application executable
#for windows application.
if(BUILD_GUI_APP)
	add_executable(application_executable WIN32 main.cpp)
	message("Windows Application")
else()
	add_executable(application_executable main.cpp)
	message("Console Application")
endif()


#Add includes and source files to executable.
add_subdirectory(Reflection)
add_subdirectory(Utility)



