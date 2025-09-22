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

#include "ObjectManager.hpp"


namespace EngineFramework
{
    ObjectManager::ObjectManager() {
        LOGI("Lfeature: ObjectManager CREATED");
    }
    ObjectManager::~ObjectManager() {

    }
    void ObjectManager::Initialize() {

    }
    void ObjectManager::Run() {

        //SAMPLE OF HOW LFEATURE WORKS

        //// Give size once; don't lock it every frame
        ImGui::SetNextWindowSize(ImVec2(420, 260), ImGuiCond_FirstUseEver);

        // No flags that block move/resize
        ImGui::Begin("Object Manager", &is_enabled);
        ImGui::Text("Drag my title bar / edges.");
        // --- per frame UI ---
        static int click_count = 0;

        if (ImGui::Button("Click me")) {
            ++click_count;
            LOGI((std::string("[ImGui] Button clicked: ") + std::to_string(click_count).c_str() + " times").c_str());
        }

        ImGui::End();

    }
    void ObjectManager::Close() {
        LOGI("object manager closed");
    }

}
#endif
