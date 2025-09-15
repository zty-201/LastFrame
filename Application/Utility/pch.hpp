/*
	Precompiled header file used to speed up compilation/build times.
	Only include pch.hpp in source (.cpp) files.
*/
#ifndef PRECOMPILED_HEADER_HPP
#define PRECOMPILED_HEADER_HPP

#include <string>
#include <memory>
#include <vector>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>
#include <random>
#include "global.hpp"
#include "ISystem.hpp"
#include "CoreEngine.hpp"

//Cross-Platform Compatibility
#include "Platform.h"

#ifdef PLATFORM_WINDOWS
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#elif defined(PLATFORM_ANDROID)
#include <glad/glad.h> //Loads and provides android es functions
#include <EGL/egl.h>
#endif


// GLM (OpenGL Mathematics)
//TODO: Replace with centralized math lib.
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/ext.hpp> // For mathematical constants like pi
#include <glm/gtc/matrix_transform.hpp> // Utility functions for matrix transformations


//// Windows API headers
//#ifdef _WIN32
//#define WIN32_LEAN_AND_MEAN  // Exclude rarely-used stuff from Windows headers
//#include <windows.h>
//#endif

#endif