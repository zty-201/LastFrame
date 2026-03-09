/*
	Precompiled header file used to speed up compilation/build times.
	Only include pch.hpp in source (.cpp) files.
*/
#ifndef PRECOMPILED_HEADER_HPP
#define PRECOMPILED_HEADER_HPP

//Cross-Platform Compatibility
#include "Platform.h"

//Standard Library
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <random>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <variant>
#include <unordered_set>
#include <tuple>
#include <set>


//Physics
//#include <Jolt/Jolt.h>


//ECS and reflection and event system
#include "entt/entity/registry.hpp"
#include "entt/entity/entity.hpp"
#include "entt/meta/meta.hpp"
#include "entt/meta/resolve.hpp"
#include "entt/meta/factory.hpp" 
#include "entt/core/hashed_string.hpp"
#include "entt/signal/dispatcher.hpp"
#include <entt/core/type_traits.hpp>


//Json parser
#include "nlohmann/json.hpp"

//Asset Management via guids
#include "xresource_guid.h"

//Opengl Rendering
#ifdef PLATFORM_WINDOWS
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#elif defined(PLATFORM_ANDROID)
#include <EGL/egl.h>
//For opengl es 3.2
#include <GLES3/gl32.h>
#include <gles3/gl32.h>
#endif


// GLM (OpenGL Mathematics)
//TODO: Replace with centralized math lib.
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/ext.hpp> // For mathematical constants like pi
#include <glm/gtc/matrix_transform.hpp> // Utility functions for matrix transformations

#ifndef DISABLE_FMOD
#include "fmod.hpp"
#include "fmod_errors.h"
#endif

//Core Systems and Global Definitions
#include "global.hpp"
#include "ISystem.hpp"
#include "CoreEngine.hpp"
#include "AssetManager.hpp"

//Commonly used systems, or Systems that don't change often.
#include "Audio.hpp"
#include "InputManager.hpp"
#include "Math.hpp"
#include "Animation.hpp"
#include "AnimationImporter.hpp"

//Event system
#include "event_structures.hpp"

#endif