#if defined(PLATFORM_WINDOWS) && defined(_DEBUG)
#include "SceneManager.hpp"
#include "System_GameObjectSystem.hpp"
#include <filesystem>
#include <iostream>

namespace EngineFramework
{
	SceneManager::SceneManager() {
		this_system = this;
		LOGI("Lfeature: SceneManager CREATED");
	}
	SceneManager::~SceneManager() {}
	void SceneManager::Initialize() {}

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

	void SceneManager::LoadScene(const std::string& scenePath)
	{
		GameObjectSystem::GetInstance().LoadScene(scenePath);
		currentScene = scenePath;
	}

	const std::string& SceneManager::GetCurrentScene() const
	{
		return currentScene;
	}

	void SceneManager::Run()
	{
		if (ImGui::Begin("Scene Manager", &is_enabled)) {
			ImGui::Text("Available Scenes:");

			for (const auto& scene : GetSceneList()) {
				if (ImGui::Selectable(scene.c_str())) {
					LoadScene(scene);
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