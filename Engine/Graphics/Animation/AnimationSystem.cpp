#include "pch.hpp"
#include "global.hpp"
#include "Debug.h"
#include "AnimationSystem.h"
#include "GraphicsSystem.h"

#if defined(DEBUG) && defined(PLATFORM_WINDOWS)
#include "System_LevelEditor.hpp"
class LevelEditor; //forward declaration
#endif

namespace EngineFramework
{
	
	

	AnimationSystem::AnimationSystem() { AnimationSystemPtr = this; }

	AnimationSystem::~AnimationSystem() { glfwTerminate(); }

}