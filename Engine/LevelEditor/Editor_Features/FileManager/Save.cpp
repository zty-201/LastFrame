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

#include "Save.hpp"


namespace EngineFramework
{
    Save::Save() {
        LOGI("Lfeature: Save CREATED");
    }
    Save::~Save() {

    }
    void Save::Initialize() {

    }
    void Save::Run() {

        //since saving is a one time event, is_enabled should be set to false when done.
        LOGI("Saved");
        is_enabled = false;


    }
    void Save::Close() {

    }

}
#endif
