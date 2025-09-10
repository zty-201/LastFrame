#Engine, the static library used by the game application.
#For reusable code not specific to this game.

add_library(engine_library STATIC)

#add source files and include files to the library.
add_subdirectory(ECS)
add_subdirectory(Graphics)
add_subdirectory(Input)
add_subdirectory(Math)
