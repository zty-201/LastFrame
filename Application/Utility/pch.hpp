/*
	Precompiled header file used to speed up compilation/build times.
	Only include pch.hpp in source (.cpp) files.
*/
#ifndef PRECOMPILED_HEADER_HPP
#define PRECOMPILED_HEADER_HPP

//Cross-Platform Compatibility
#include "Platform.h"
#include <string>
#include <memory>
#include <vector>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <random>
#include <map>
#include <functional>

//Asset Management via guids
#include "xresource_guid.h"

//Core Systems and Global Definitions
#include "global.hpp"
#include "ISystem.hpp"
#include "CoreEngine.hpp"


//Opengl Rendering
#ifdef PLATFORM_WINDOWS
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#elif defined(PLATFORM_ANDROID)
#include <EGL/egl.h>
//For opengl es 3.2
#include <GLES3/gl32.h>
#endif


// GLM (OpenGL Mathematics)
//TODO: Replace with centralized math lib.
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/ext.hpp> // For mathematical constants like pi
#include <glm/gtc/matrix_transform.hpp> // Utility functions for matrix transformations

//Event system
#include "event_structures.hpp"
//// Windows API headers
//#ifdef _WIN32
//#define WIN32_LEAN_AND_MEAN  // Exclude rarely-used stuff from Windows headers
//#include <windows.h>
//#endif

#endif