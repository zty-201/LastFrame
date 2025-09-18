A 3D Horror Game

Made by Team Cheerios as part of our 3rd year project.



Build Process when changing file/folder structure.
Includes:
- Adding a new library
- Adding/Deleting a new file
- Renaming a file
- Moving a file
- Adding a folder
- Pulling git (files may have changed)
0. Ensure no folder path has spaces in them.
- The CMake tries to account for this, but it's not made for it.
1. Add library to CMake if necessary
- Download library and put under the correct Lib/ folder.
- Add library target + target_include PUBLIC for this target, to cmake file in your library, add_subdirectory to your library directory, add the target under top-level cmake file as an external dependency.
- Joel knows how to do this.
2. Run the configuration .bat file for your specific IDE in the top-level directory (double click)
2. Open up build/{IDE Name}, and find your IDE there
3. Run the IDE to build code.
4. Executable will be in the build folder build/{IDE Name}/{Release or Debug}

Note: Anything in the build/ folder is TEMPORARY.
Includes:
- Changing solution/project properties
- Extern/ folder
- And anything else.

Cleaning:
1. Run the clean .bat file, or delete build/ folder.


#TODO:
1. Cross-Platform input handling, independent of glfw.
2. Cross-Platform graphics, independent of glfw.
3. Cross-Platform Asset manager (use AAsset to get assets/)
Note: Filepath may be different depending on windows vs android (use / for windows and android). 