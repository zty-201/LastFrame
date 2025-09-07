/*
	Precompiled header file used to speed up compilation/build times.
	Only include pch.hpp in source (.cpp) files.
*/
#ifndef PRECOMPILED_HEADER_HPP
#define PRECOMPILED_HEADER_HPP

// Windows API headers
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN  // Exclude rarely-used stuff from Windows headers
#include <windows.h>
#endif

// GLEW & GLFW (OpenGL Context & Extensions)
#include <GLAD/glad.h>
#include <GLFW/glfw3.h>

// GLM (OpenGL Mathematics)
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <string>
#include <memory>
#include <vector>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>
#include "macros.hpp"
#include "ISystem.hpp"
#include "CoreEngine.hpp"

#endif