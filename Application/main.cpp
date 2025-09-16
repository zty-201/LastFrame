/******************************************************************************/
/*!
\file		main.cpp
\author 	Joel Lee Jie
\par    	joeljie.lee\@digipen.edu
\brief  	Entry point of the application

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the prior
written consent of DigiPen Institute of Technology is prohibited.
*/
/******************************************************************************/

#include "pch.hpp"
#include <iostream>
//Measurement of delta time.
#include <chrono>
#include "main.hpp"
#include "GraphicsSystem.h" // Include GraphicsSystem header
#include "Input/InputManager.hpp"
#include "ReflectionRegister.hpp"
#include "System_GameObjectSystem.hpp"
#include "Assets/AssetManager.hpp"

#if defined(PLATFORM_WINDOWS) && defined(_DEBUG)
#include "System_LevelEditor.hpp"
#endif




void Game_Initialize()
{
	/*
			Memory fault checks
			1. crtdbg.h
			- Checks for mem leaks during run-time, run in debugger mode.
			- Disable asan if using.

			2. Asan
			- Checks for mem fault locations like double deletion, use after free, buffer overflow etc.
			- May have to disable incremental linking, hot reloading (and maybe /RTC1) before use.

			To disable asan, temporarily comment out /fsanitize=address in cmake file, then enable it back after checking
		*/
		// Enable run-time memory check for debug builds.
		//Disable /fsanitize=address temporarily to use, remember to enable afterwards
#if defined(DEBUG) && defined(PLATFORM_WINDOWS)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
#endif

	/*
			Process
			1. Initialize all external libraries that cannot be initialized normally (maybe the initialization order is finicky).
			2. Add systems to CoreEngine
			3. Initialize systems
			4. While loop (while the opengl window is open and has not received request to close).
			- Get delta time
			- Calculate FPS (not using delta time, but rather counting the number of frames/s
			- Update systems
			- PostUpdate Systems
			- Limit FPS if needed (possibly by sleeping the pc).
			6. Deallocate external libraries that aren't deallocated in the next step
			7. Deallocate CoreEngine and all attached systems. Due to RAII, it'll be automatically done when the program ends so nothing needs to be done here.
			- Note: Sometimes this step must be done before deallocation of external libraries. If so, CoreEngine should not be deallocated via RAII but rather via a manual function call before the previous step.
		*/
		//Initialize Entt Reflection System by registering all components.
	EngineFramework::RegisterReflection();
	EngineFramework::CoreEngine& core_engine = EngineFramework::CoreEngine::GetInstance();

	//=Adding Systems
	core_engine.AddSystem("AssetManager", std::make_unique<EngineFramework::AssetManagerSystem>(), 1, 1, 100);
	core_engine.AddSystem("GameObjectSystem", std::make_unique<EngineFramework::GameObjectSystem>(), 2, 2, 1);
	// Add the GraphicsSystem to the CoreEngine
	core_engine.AddSystem(
		"GraphicsSystem",            // Name
		std::make_unique<EngineFramework::GraphicsSystem>(),  // Transfer ownership to CoreEngine
		1,                          // Init order (low = init first)
		1,                          // Update order (low = update first)
		3                         // Destruct order (high = destroy last)
	);

	

	core_engine.AddSystem("AssetManager", std::make_unique<EngineFramework::AssetManagerSystem>(), 3, 1, 4);

	core_engine.AddSystem("InputManager", std::make_unique<EngineFramework::InputManager>(), 2, 1, 3);

#if defined(DEBUG) && defined(PLATFORM_WINDOWS)
	core_engine.AddSystem("LevelEditorSystem", std::make_unique<EngineFramework::LevelEditorSystem>(), 1, 1, 1);
#endif




	//=Initialize Systems
	try
	{
		core_engine.InitializeSystems();
	}
	catch (std::exception& e)
	{
		std::cout << "Game Aborted due to failure to initialize systems.\n"
			<< e.what() << std::endl;
		std::abort();
	}
}

void Game_Update()
{
	EngineFramework::CoreEngine& core_engine = EngineFramework::CoreEngine::GetInstance();
	//Keeps track of the current delta time. Use a small arbitrary value for the first update loop.
	static float delta_time{ 0.0001f };
	static std::chrono::steady_clock::time_point time_before_loop{};
	static std::chrono::steady_clock::time_point time_after_loop{};
	time_before_loop = std::chrono::steady_clock::now();

	//TODO: Try-Catch handling in Update/PostUpdate.
	core_engine.UpdateSystems(delta_time);

	core_engine.PostUpdateSystems(delta_time);

	time_after_loop = std::chrono::steady_clock::now();

	//Convert microseconds into seconds, set the delta time.
	delta_time = static_cast<float>(std::chrono::duration_cast<std::chrono::microseconds>(time_after_loop - time_before_loop).count()) / 1000000.f;
	//Limit the delta time value.
	delta_time = (delta_time < MAX_DELTA_TIME) ? delta_time : MAX_DELTA_TIME;
}

void Game_Destroy()
{

}
