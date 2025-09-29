#if defined(PLATFORM_WINDOWS) && defined(_DEBUG)
#ifndef SCENEMANAGER_LFEATURE_HPP
#define SCENEMANAGER_LFEATURE_HPP

#include <unordered_map>
#include <string>
#include <sstream>
#include "ISystem.hpp"
#include "entt/entity/registry.hpp"
#include "System_LevelEditor.hpp"
#include "LFeature.hpp"

namespace EngineFramework
{
    struct iEditorView; //forward declaration

    class SceneManager : public LFeature
    {
    public:

        SceneManager();
        ~SceneManager() override;

        void Initialize() override;
        void Run() override;
        void Close() override;

        static SceneManager& GetInstance();

        static bool feature_enabled;

        const std::vector<std::string>& GetSceneList() const;
        void ScanScenes(const std::string& dir);
        void LoadScene(const std::string& scenePath);
        const std::string& GetCurrentScene() const;

    private:
        static inline SceneManager* this_system{};
        std::vector<std::string> sceneList;
        std::string currentScene;
    };
}

#endif
#endif