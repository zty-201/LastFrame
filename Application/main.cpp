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
//Measurement of delta time.
#include <chrono>
// For the profiler
#include <iostream>
#include <vector>
#include <algorithm>
#include <iomanip>

#include "main.hpp"
#include "GraphicsSystem.h" // Include GraphicsSystem header
#include "Input/InputManager.hpp"
#include "ReflectionRegister.hpp"
#include "System_GameObjectSystem.hpp"
#include "Assets/AssetManager.hpp"
#include "Diagnostics/ProfilerEngine.hpp" // For the profiler - engine side
#include "Audio/Audio.hpp"

#if defined(PLATFORM_WINDOWS) && defined(_DEBUG)
#include "System_LevelEditor.hpp"
#endif


//Initialization of game application.
//android_asset_manager should only be set when building for android.
void Game_Initialize(void* android_asset_manager)
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
#ifdef PLATFORM_WINDOWS
	UNREFERENCED_PARAMETER(android_asset_manager);
#elif defined(PLATFORM_ANDROID)
	//Set the asset manager.
	if (!android_asset_manager)
	{
		throw std::runtime_error("No AAsetManager asset manager passed to Game_Initialize() function");
	}
	EngineFramework::AssetManagerSystem::SetAndroidAssetManager(android_asset_manager);
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

	// --- Core Systems (Init First, Destroy Last) ---

	// 1. GraphicsSystem: Initializes the window and graphics context. Must be first.
		core_engine.AddSystem(
			"GraphicsSystem",
			std::make_unique<EngineFramework::GraphicsSystem>(),
			1,  // Init Order: First
			5,  // Update Order: After game logic and editor
			5   // Destruct Order: Last
		);

		// 2. InputManager: Needs the window from GraphicsSystem to register callbacks.
		core_engine.AddSystem(
			"InputManager",
			std::make_unique<EngineFramework::InputManager>(),
			2,  // Init Order: After Graphics
			1,  // Update Order: FIRST in the loop
			4   // Destruct Order: Before Graphics
		);

		// 3. AssetManager: Can load assets after core systems are up.
		core_engine.AddSystem(
			"AssetManager",
			std::make_unique<EngineFramework::AssetManagerSystem>(),
			3,  // Init Order
			2,  // Update Order: After Input
			3   // Destruct Order
		);

		// --- Game Logic and Other Systems ---

		// 4. GameObjectSystem: The main game logic.
		core_engine.AddSystem(
			"GameObjectSystem",
			std::make_unique<EngineFramework::GameObjectSystem>(),
			4,  // Init Order
			3,  // Update Order: After Input and Asset Management
			2   // Destruct Order
		);

		// 5. AudioSystem: Depends on the game state.
		core_engine.AddSystem(
			"AudioSystem",
			std::make_unique<EngineFramework::AudioSystem>(),
			5,  // Init Order
			6,  // Update Order: Last
			1   // Destruct Order: First
		);

		// --- Debug/Editor Systems (Conditional) ---

	#if defined(DEBUG) && defined(PLATFORM_WINDOWS)
	// 6. LevelEditorSystem: Controls rendering when active.
		core_engine.AddSystem(
			"LevelEditorSystem",
			std::make_unique<EngineFramework::LevelEditorSystem>(),
			100,  // Init Order: After all other game systems
			7,  // Update Order: After game logic and main graphics update
			1   // Destruct Order: With other game systems
		);
	#endif

	//=Initialize Systems
	try
	{
		core_engine.InitializeSystems();
	}
	catch (std::exception& e)
	{
		LOGE("Game Aborted due to failure to initialize systems.");
		LOGE("%s", e.what());
		std::abort();
	}
}

void Game_Update()
{
	// TESTING PROFILER 
	using Diagnostics::Profiler;
	auto& prof = Profiler::I(); // To get the profiler

	EngineFramework::CoreEngine& core_engine = EngineFramework::CoreEngine::GetInstance();
	//Keeps track of the current delta time. Use a small arbitrary value for the first update loop.
	static float delta_time{ 0.0001f };
	static std::chrono::steady_clock::time_point time_before_loop{};
	static std::chrono::steady_clock::time_point time_after_loop{};
	time_before_loop = std::chrono::steady_clock::now();

#ifdef PLATFORM_WINDOWS
	//Input update for windows.
	glfwPollEvents();
#endif

	// START PROFILER frame timing
	prof.BeginFrame(); 
	{
		Diagnostics::Scoped s{ "Frame::UpdateAll" };


		//TODO: Try-Catch handling in Update/PostUpdate.
		core_engine.UpdateSystems(delta_time);

		core_engine.PostUpdateSystems(delta_time);
	}

	prof.EndFrame(); // END PROFILER frame timing

	// Print profiler summary
	static double acc_ms = 0.0;
	static int frames = 0;
	acc_ms += Profiler::I().FrameMS();
	frames++;

	if (acc_ms >= 1000.0) {
		auto& P = Profiler::I();
		const double frame = std::max(1e-6, P.FrameMS());

		double update_all = frame;
		if (auto it = P.Sections().find("Frame::UpdateAll"); it != P.Sections().end()) {
			update_all = std::max(1e-6, it->second.ms);
		}

		std::cout << std::fixed << std::setprecision(4);
		std::cout << "[Profiler] Frame: " << P.FrameMS() << " ms, FPS " << std::setprecision(1) << P.FPS() << "\n";
		std::cout << std::setprecision(4);

		// Copy and sort the sections by time in descending order
		std::vector<std::pair<std::string, Diagnostics::Section>> rows(P.Sections().begin(), P.Sections().end());
		//std::sort(rows.begin(), rows.begin(), [](auto& a, auto& b) {return a.second.ms > b.second.ms; });

		for (auto& [name, sec] : rows) {
			const double frame_percent = (sec.ms / frame) * 100.0;
			const double engine_update_percent = (sec.ms / update_all) * 100.0;
			const double per_call = sec.calls ? sec.ms / sec.calls : 0.0;

			std::cout << " " << name << " : " << sec.ms << " ms" << " (calls " << sec.calls << ", " << per_call << " ms/call)"
				<< " [" << std::setprecision(1) << engine_update_percent << "% of UpdateAll, " << frame_percent << "% of Frame]\n" << std::setprecision(4);
		}
		std::cout << "--------------------------------------\n";
		acc_ms = 0.0;

	}

	//if (acc_ms >= 1000.0) {
	//	std::cout << "[Profiler] Frame: " << Profiler::I().FrameMS() << " ms, FPS " << Profiler::I().FPS() << "\n";

	//	for (const auto& kv : Profiler::I().Sections()) {
	//		const auto& name = kv.first;
	//		const auto& sec = kv.second;
	//		const double per_call = (sec.calls ? sec.ms / sec.calls : 0.0);
	//		std::cout << " " << name << " : " << sec.ms << " ms" << " (calls " << sec.calls << ", " << per_call << " ms/call)\n";
	//	}
	//	std::cout << "--------------------------------------\n";
	//	acc_ms = 0.0;
	//	frames = 0;
	//}

	//Send all queued events in order.
	event_dispatcher.update();

	time_after_loop = std::chrono::steady_clock::now();

	//Convert microseconds into seconds, set the delta time.
	delta_time = static_cast<float>(std::chrono::duration_cast<std::chrono::microseconds>(time_after_loop - time_before_loop).count()) / 1000000.f;
	//Limit the delta time value.
	delta_time = (delta_time < MAX_DELTA_TIME) ? delta_time : MAX_DELTA_TIME;
}

void Game_Destroy()
{

}
