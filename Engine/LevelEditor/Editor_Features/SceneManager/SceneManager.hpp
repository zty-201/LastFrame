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

        static bool feature_enabled;
    };
}

#endif
#endif