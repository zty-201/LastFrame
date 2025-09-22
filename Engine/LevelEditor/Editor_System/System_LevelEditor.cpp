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

//Casting meta data to type.
#include "ReflectionRegister.hpp"
#include "InputManager.hpp"
#include "Level_Editor.hpp"

namespace EngineFramework
{
	bool LevelEditorSystem::Get_Enable() {
		return LevelEditor::Get_Editor_State();
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
	}
	void LevelEditorSystem::Update(float delta_time)
	{
		UNREFERENCED_PARAMETER(delta_time);

		//only check for this if the editor state is not true
		if (!application->Get_Editor_State()) {
			//std::cout << "yo\n";
			if (InputManager::GetInstance().GetKeyDown(GLFW_KEY_1)) {

				application->Initialize();
				application->Open();
				//std::cout << "2222222222222222222222222222222222222\n";
			}
		}
		else {

			if (application->To_Close()) {

				application->Close();
				application->Clean();

			}
			else if (application->Get_Editor_State()) {
				//std::cout << "Imgui running\n";
				//open is a 1 time thing, it shouldnt be called everyframe
				//see if there are things extra things to init in imgui, thats 1 time when running, else open can be removed			
				application->Run();

			}
		}

	}

}
#endif

