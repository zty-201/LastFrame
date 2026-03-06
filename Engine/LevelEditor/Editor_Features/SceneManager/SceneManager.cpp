/*
*****************************************************************/
/*!
\file   SceneManager.cpp
\author Tang Yeun, Dean
\par    y.tang\@digipen.edu
\date   September 2025
\brief
This file allows the user to change scenes / levels
Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* *******************************************************************/
#if defined(PLATFORM_WINDOWS) && defined(_DEBUG)
#include "pch.hpp"
#include "SceneManager.hpp"
#include "System_GameObjectSystem.hpp"
#include <filesystem>
#include <iostream>
#include "AssetManager.hpp"
#include "Screen.hpp"
#include "UISystem.hpp"
#include "ObjectManager.hpp"

namespace EngineFramework
{
	//defualt scene to load
	std::string defaultScene = "M4_Level.json";
	bool SceneManager::is_file_loaded = false;


	SceneManager::SceneManager() {
		this_system = this;
		LOGI("Lfeature: SceneManager CREATED");
	}
	SceneManager::~SceneManager() {}

	void SceneManager::Initialize()
	{
		//need to remove this in later build, files should not be loaded as default
		ScanScenes(AssetManagerSystem::GetInstance().GetAssetFolderPrefix().string() + "Extern/Scenes/");

		std::string sceneString = AssetManagerSystem::GetStringFromFile("Extern/Scenes/" + defaultScene);
		GameObjectSystem::GetInstance().LoadScene(sceneString);
		UI_System::GetInstance().Initialize();
		UI_System::GetInstance().closeAllMenu();

		currentScene = defaultScene;
		ObjectManager::LoadFiltersForScene(currentScene);
		Screen::GetInstance().is_enabled = true;
		is_file_loaded = true;
	}

	SceneManager& SceneManager::GetInstance()
	{
		return *this_system;
	}

	const std::vector<std::string>& SceneManager::GetSceneList() const
	{
		return sceneList;
	}

	void SceneManager::ScanScenes(const std::string& dir)
	{
		sceneList.clear();
		for (const auto& entry : std::filesystem::directory_iterator(dir))
		{
			if (entry.is_regular_file())
			{
				auto path = entry.path();
				if (path.extension() == ".json")
				{
					sceneList.push_back(path.filename().string());
				}
			}
		}
	}

	const std::string& SceneManager::GetCurrentScene() const
	{
		return currentScene;
	}

	void SceneManager::Run()
	{
		if (!is_enabled) return; // only open if toggled

		if (ImGui::Begin("Scene Manager", &is_enabled))
		{
			ImGui::Text("Available Scenes:");
			ImGui::Separator();
			for (const auto& scene : GetSceneList())
			{
				if (ImGui::Selectable(scene.c_str()))
				{
					LOGD("Selected scene: %s", scene.c_str());
					std::string sceneString = AssetManagerSystem::GetStringFromFile("Extern/Scenes/" + scene);
					GameObjectSystem::GetInstance().LoadScene(sceneString);
					currentScene = scene;
					UI_System::GetInstance().Initialize();
					Screen::GetInstance().is_enabled = true;
					ObjectManager::LoadFiltersForScene(currentScene);
					is_file_loaded = true;
				}
			}
		}
		ImGui::End();
	}

	void SceneManager::Close()
	{
		LOGI("Scene manager closed");
	}
}
#endif
