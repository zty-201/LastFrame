/*
*****************************************************************/
/*!
\file   System_LevelEditor.cpp
\author Tan Yee Tong
\par    yeetong.t\@digipen.edu
\date   September 2025
\brief
This file is the level editor system, where the system will call the level_editor application
Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* *******************************************************************/


#if defined(PLATFORM_WINDOWS) && defined(_DEBUG)
#include "pch.hpp"

//File management, JSON file parser/writer
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

//Entt reflection system
#include <entt/meta/meta.hpp>
#include <entt/meta/resolve.hpp>
//Entt ECS system
#include <entt/entity/registry.hpp>
#include "System_LevelEditor.hpp"
#include <chrono>

//Casting meta data to type.
#include "ReflectionRegister.hpp"
#include "InputManager.hpp"
#include "Level_Editor.hpp"
#include "AssetManager.hpp"
namespace EngineFramework
{
	namespace {
		using Clock = std::chrono::high_resolution_clock;

		struct ScopedSectionTimer {
			LevelEditorSystem::ProfilerSectionSample& section;
			Clock::time_point start;
			ScopedSectionTimer(LevelEditorSystem::ProfilerSectionSample& target, bool count_call)
				: section(target), start(Clock::now())
			{
				section.valid = true;
				if (count_call) {
					section.calls = 1;
				}
			}

			~ScopedSectionTimer()
			{
				section.ms = std::chrono::duration<double, std::milli>(Clock::now() - start).count();
			}
		};

		LevelEditorSystem::ProfilingData g_level_editor_profiling{};
	}

	bool LevelEditorSystem::Get_Enable() {
		return LevelEditor::Get_Editor_State();
	}

	const LevelEditorSystem::ProfilingData& LevelEditorSystem::GetProfilingData()
	{
		return g_level_editor_profiling;
	}

	LevelEditorSystem::LevelEditorSystem()
	{
		this_system = this;

	}

	void LevelEditorSystem::Initialize()
	{
		application = std::make_unique<LevelEditor>();

		//application->Initialize();
		//application->Open();
		LOGI("level editor system ONLINE!");

		Event_Level_Editor_Toggled editor_toggled{};
		editor_toggled.is_enabled = false;
		event_dispatcher.trigger<Event_Level_Editor_Toggled>(std::move(editor_toggled));
	}
	void LevelEditorSystem::Update(float delta_time)
	{
		auto& profiling = g_level_editor_profiling;
		profiling.update.valid = false;
		profiling.update.calls = 0;
		profiling.run.valid = false;
		profiling.run.calls = 0;
		ScopedSectionTimer update_timer{ profiling.update, true };
		UNREFERENCED_PARAMETER(delta_time);

		if (!application)return;

		//only check for this if the editor state is not true
		if (!application->Get_Editor_State()) {
			//std::cout << "yo\n";



			if (InputManager::GetInstance().GetKeyDown(Key_Actions::OPEN_EDITOR)) {



				application->Initialize();
				application->Open();

				// Send an enabled event.
				Event_Level_Editor_Toggled editor_toggled{};
				editor_toggled.is_enabled = true;
				event_dispatcher.trigger<Event_Level_Editor_Toggled>(std::move(editor_toggled));

			}
		}
		else {

			if (application->To_Close()) {

				application->Close();
				application->Clean();

				// Send a disabled event.
				Event_Level_Editor_Toggled editor_toggled{};
				editor_toggled.is_enabled = false;
				event_dispatcher.trigger<Event_Level_Editor_Toggled>(std::move(editor_toggled));

				// Send an resize event.
				Event_Window_Resized editor_resize{};
				editor_resize.width = InputManager::GetInstance().GetWindowSize().first;
				editor_resize.height = InputManager::GetInstance().GetWindowSize().second;
				event_dispatcher.trigger<Event_Window_Resized>(std::move(editor_resize));
			}
				else if (application->Get_Editor_State()) {
					//std::cout << "Imgui running\n";
					//open is a 1 time thing, it shouldnt be called everyframe
					//see if there are things extra things to init in imgui, thats 1 time when running, else open can be removed			
					ScopedSectionTimer run_timer{ profiling.run, true };
					application->Run();

				}
		}

	}

}
#endif
