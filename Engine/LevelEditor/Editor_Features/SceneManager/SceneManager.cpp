#include "SceneManager.hpp"

namespace EngineFramework
{
	SceneManager::SceneManager() {
		LOGI("Lfeature: SceneManager CREATED");
	}
	SceneManager::~SceneManager() {}
	void SceneManager::Initialize() {}

	void SceneManager::Run()
	{

	}

	void SceneManager::Close()
	{
		LOGI("Scene manager closed");
	}
}