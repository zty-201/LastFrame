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
#include <vector>
#include <iostream>
#include <iomanip>
#if defined(_DEBUG) && defined(PLATFORM_WINDOWS)
#include <crtdbg.h>
#endif

#include "main.hpp"
#include "GraphicsSystem.h" // Include GraphicsSystem header
#include "Input/InputManager.hpp"
#include "ReflectionRegister.hpp"
#include "System_GameObjectSystem.hpp"
#include "AssetManager.hpp"
#include "Diagnostics/ProfilerEngine.hpp" // For the profiler - engine side
#include "UnitTestManager.hpp"
#include "AssetManager.hpp"
#include "UISystem.hpp"
#include "Audio/Audio.hpp"
#include "PhysicsRuntimeSystem.hpp"
#include "AnimationSystem.h"
#include "ScriptWrapper.hpp"
#include "StateMachineSystem.hpp"

#if defined(PLATFORM_WINDOWS) && defined(_DEBUG)
#include "System_LevelEditor.hpp"
#endif

//#if defined(ENGINE_ENABLE_LUA)
//void TestLuaFunctions() {
//	LOGI("=== Testing Lua standalone functions ===");
//
//	try {
//		sol::state& lua = Engine::ScriptSystem::GetLuaState();
//
//		// Load the test script into global space (not in a component)
//		lua.script_file("Extern/Scripts/test.lua");
//
//		// Call the greet function
//		sol::function greet = lua["greet"];
//		if (greet.valid()) { //Continues if greet function is added correctly
//			std::string result = greet("C++ Engine");
//			LOGI("Greet returned: %s", result.c_str());
//		}
//
//		// Call the add function
//		sol::function add = lua["add"];
//		if (add.valid()) { //Continue if add function is added correctly
//			int sum = add(15, 27);
//			LOGI("Add returned: %d", sum);
//		}
//
//		LOGI("=== Lua function test complete ===");
//	}
//	catch (const sol::error& e) {
//		LOGE("Lua test error: %s", e.what());
//	}
//}
//#endif
//// End of Jerron's Scripting testing



// For submission: only 1 scene, hardcode it.
static const char* g_MainSceneFile = "M4_Level.json";



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
#if defined(_DEBUG) && defined(PLATFORM_WINDOWS)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
	//_CrtSetBreakAlloc(11183);
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
			9,  // Update Order: After game logic and editor
			8   // Destruct Order: Last
		);

		// 2. InputManager: Needs the window from GraphicsSystem to register callbacks.
		core_engine.AddSystem(
			"InputManager",
			std::make_unique<EngineFramework::InputManager>(),
			2,  // Init Order: After Graphics
			1,  // Update Order: FIRST in the loop
			6   // Destruct Order: Before Graphics
		);

		core_engine.AddSystem(
			"StateMachineSystem",
			std::make_unique<EngineFramework::StateMachineSystem>(),
			10,
			2,
			5
		);

		// 3. AudioSystem: Depends on the game state.
		core_engine.AddSystem(
			"AudioSystem",
			std::make_unique<EngineFramework::AudioSystem>(),
			3,  // Init Order
			10,  // Update Order: Last
			5   // Destruct Order: First
		);

		// 4. AssetManager: Can load assets after graphics and audio systems are up.
		core_engine.AddSystem(
			"AssetManager",
			std::make_unique<EngineFramework::AssetManagerSystem>(),
			4,  // Init Order
			5,  // Update Order
			4   // Destruct Order
		);

		// 5. PhysicsRuntimeSystem: Handles all physics updates and Jolt integration. (Create & Syncing ECS entites with the physics world.)
		core_engine.AddSystem(
			"PhysicsRuntimeSystem",
			std::make_unique<EngineFramework::PhysicsRuntimeSystem>(),
			5,  // Init Order: after AssetManager, before GameObjectSystem
			4,  // Update Order: after Input, before GameObjectSystem & Graphics
			3   // Destruct Order: before GameObjectSystem/Graphics are gone
		);

		// --- Game Logic and Other Systems ---

		// 6. GameObjectSystem: The main game logic.
		core_engine.AddSystem(
			"GameObjectSystem",
			std::make_unique<EngineFramework::GameObjectSystem>(),
			6,  // Init Order
			6,  // Update Order: After Input and Asset Management
			2   // Destruct Order
		);

		// 7. ScriptSystem: Game scripting logic.
		core_engine.AddSystem(
			"ScriptWrapper",
			std::make_unique<EngineFramework::ScriptWrapper>(),
			7,  // Init Order: after GameObjectSystem
			3,  // Update Order: after InputManager
			2   // Destruct Order: before GameObjectSystem is gone
		);

//#if defined(DEBUG) && defined(PLATFORM_WINDOWS)
		// 8. LevelEditorSystem: Controls rendering when active.
		core_engine.AddSystem(
			"UI_System",
			std::make_unique<EngineFramework::UI_System>(),
			8,  // Init Order: after AssetManager, before GameObjectSystem
			7,  // Update Order: after Input, before GameObjectSystem & Graphics
			3   // Destruct Order: before GameObjectSystem/Graphics are gone
		);
//#endif

		// 9. AnimationSystem: For animation to work.
		core_engine.AddSystem(
			"AnimationSystem",
			std::make_unique<EngineFramework::AnimationSystem>(),
			9,	// Init Order
			8,	// Update Order
			7	// Destruct Order
		);

		// --- Debug/Editor Systems (Conditional) ---

	#if defined(_DEBUG) && defined(PLATFORM_WINDOWS)
	// 10. LevelEditorSystem: Controls rendering when active.
		core_engine.AddSystem(
			"LevelEditorSystem",
			std::make_unique<EngineFramework::LevelEditorSystem>(),
			100,  // Init Order: After all other game systems
			11,  // Update Order: After game logic and main graphics update
			1   // Destruct Order: With other game systems
		);
	#endif

	//=Initialize Systems
	try
	{
		core_engine.InitializeSystems();

#ifdef DEBUG
		// Creates the only instance in the program, and runs all unit tests.
		UnitTestManager::GetInstance().RunAllTests();
#endif
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
	// 1. Keep track of the previous frame's timestamp across function calls
	static auto last_time = std::chrono::steady_clock::now();

	// 2. Get the current timestamp right as the frame begins
	auto current_time = std::chrono::steady_clock::now();

	// 3. Calculate delta time based on the difference (in seconds)
	static float delta_time = 0.0166f; // Safe default for the very first frame
	delta_time = std::chrono::duration<float>(current_time - last_time).count();

	// 4. Immediately update last_time for the next frame's calculation
	last_time = current_time;

	// Limit the delta time value to prevent massive physics explosions on lag spikes
	delta_time = (delta_time < MAX_DELTA_TIME) ? delta_time : MAX_DELTA_TIME;

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
	//static double acc_ms = 0.0;
	//static int frames = 0;
	//acc_ms += Profiler::I().FrameMS();
	//frames++;

	//if (acc_ms >= 1000.0) {
	//	std::stringstream out_stream{};
	//	auto& P = Profiler::I();
	//	const double frame = std::max(1e-6, P.FrameMS());

	//	double update_all = frame;
	//	if (auto it = P.Sections().find("Frame::UpdateAll"); it != P.Sections().end()) {
	//		update_all = std::max(1e-6, it->second.ms);
	//	}

	//	out_stream << std::fixed << std::setprecision(4);
	//	out_stream << "[Profiler] Frame: " << P.FrameMS() << " ms, FPS " << std::setprecision(1) << P.FPS() << "\n";
	//	out_stream << std::setprecision(4);

	//	// Copy and sort the sections by time in descending order
	//	std::vector<std::pair<std::string, Diagnostics::Section>> rows(P.Sections().begin(), P.Sections().end());
	//	//std::sort(rows.begin(), rows.begin(), [](auto& a, auto& b) {return a.second.ms > b.second.ms; });

	//	for (auto& [name, sec] : rows) {
	//		const double frame_percent = (sec.ms / frame) * 100.0;
	//		const double engine_update_percent = (sec.ms / update_all) * 100.0;
	//		const double per_call = sec.calls ? sec.ms / sec.calls : 0.0;

	//		out_stream << " " << name << " : " << sec.ms << " ms" << " (calls " << sec.calls << ", " << per_call << " ms/call)"
	//			<< " [" << std::setprecision(1) << engine_update_percent << "% of UpdateAll, " << frame_percent << "% of Frame]\n" << std::setprecision(4);
	//	}
	//	out_stream << "--------------------------------------\n";
	//	acc_ms = 0.0;
	//	LOGD("%s", out_stream.str().c_str());
	//}

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

	if (EngineFramework::CoreEngine::g_RequestQuit)
	{
		EngineFramework::CoreEngine::g_RequestQuit = false;
		LOGI("[App] Quit requested");

		// Stop runtime cleanly (optional but recommended)
		Engine::ScriptSystem::OnRuntimeStop(registry);

#ifdef PLATFORM_WINDOWS
		// Prefer using your GraphicsSystem's window getter if you have one
		GLFWwindow* win = glfwGetCurrentContext();
		if (win) glfwSetWindowShouldClose(win, GLFW_TRUE);
		else std::exit(0); // fallback if window isn't accessible

#elif defined(PLATFORM_ANDROID)
		std::exit(0);
#endif

		return;
	}


	// After systems update/postupdate
	if (EngineFramework::CoreEngine::g_RequestSceneRestart)
	{
		EngineFramework::CoreEngine::g_RequestSceneRestart = false;

		LOGI("[App] Restart requested -> reloading %s", g_MainSceneFile);

		// 0) STOP scripting runtime (IMPORTANT)
		Engine::ScriptSystem::OnRuntimeStop(registry);

		// 1) Close UI first
		EngineFramework::UI_System::GetInstance().closeAllMenu();

		// 2) Clear gameplay entities
		EngineFramework::GameObjectSystem::GetInstance().ClearAllInstances();

		// 2.5) CLEAR PHYSICS BODIES (IMPORTANT) - prevents old Jolt bodies snapping player back
		PhysicsWorld::Get().DestroyAllBodies();

		// 3) Load scene JSON again
		std::string sceneString =
			EngineFramework::AssetManagerSystem::GetStringFromFile(
				std::string("Extern/Scenes/") + g_MainSceneFile
			);

		EngineFramework::GameObjectSystem::GetInstance().LoadScene(sceneString);

		// 4) START scripting runtime again (IMPORTANT)
		Engine::ScriptSystem::OnRuntimeStart(registry);

		// 5) Re-init UI and return to StartMenu
		EngineFramework::UI_System::GetInstance().Initialize();
		EngineFramework::UI_System::GetInstance().closeAllMenu();
		EngineFramework::UI_System::GetInstance().displayMenu("StartMenu");
	}

	//time_after_loop = std::chrono::steady_clock::now();

	////Convert microseconds into seconds, set the delta time.
	//delta_time = static_cast<float>(std::chrono::duration_cast<std::chrono::microseconds>(time_after_loop - time_before_loop).count()) / 1000000.f;
	////Limit the delta time value.
	//delta_time = (delta_time < MAX_DELTA_TIME) ? delta_time : MAX_DELTA_TIME;
}

void Game_Destroy()
{
	// ADD ANY DESTROY LOGIC HERE
}
