/*
*****************************************************************/
/*!
\file   ScriptDebug.cpp
\author Teng Jun Rong Jerron
\par    t.junrongjerron\@digipen.edu
\date   October 2025
\brief
This file contains the definitions of the functions that are needed for
handling the debugging of scripts.
Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* *******************************************************************/
#include "ScriptDebug.hpp"
#include "Scripting/ScriptAsset.hpp" //Testing GUID of script asset
#if defined(ENGINE_ENABLE_LUA) && defined(DEBUG)
#include "Script.hpp"
#include "InputManager.hpp"
#include <sol/sol.hpp>
namespace Engine {
    static bool s_TestInitialized = false;
    static sol::function s_GreetFunc;
    static sol::function s_AddFunc;
   // struct ScriptAsset
   // {
   //     Script()
   //     {
   //         sol::state& lua = ScriptSystem::GetLuaState();
   //         // Load the test script once during initialization
   //         auto& AM = EngineFramework::AssetManagerSystem::GetInstance();
   //         std::vector<unsigned char> bytes = AM.GetDataFromFile("Extern/Scripts/test.lua");
   //         std::string code(bytes.begin(), bytes.end());
   //         chunk = lua.load_buffer(bytes.data(), bytes.size());
			//chunk(); // execute
   //     }
   //     sol::load_result chunk{};
   // };

   // struct ScriptComponent
   // {
   //     xresource::instance_guid script_guid;
   // };
   // 
   // WORKING CODE BELOW
    //void ScriptDebug::Initialize() {
    //    LOGI("[ScriptDebug] Lua script testing enabled. Press K/L to run test script.");
    //    try {
    //        sol::state& lua = ScriptSystem::GetLuaState();

    //        // Load the test script once during initialization
    //        auto& AM = EngineFramework::AssetManagerSystem::GetInstance();
    //        std::vector<unsigned char> bytes = AM.GetDataFromFile("Extern/Scripts/test.lua");
    //        std::string code(bytes.begin(), bytes.end());
    //        auto chunk = lua.load(code, "Extern/Scripts/test.lua");
    //        chunk(); // execute

    //        // Cache function references
    //        s_GreetFunc = lua["greet"];
    //        s_AddFunc = lua["add"];

    //        if (!s_GreetFunc.valid() || !s_AddFunc.valid()) {
    //            LOGE("[ScriptDebug] Failed to load test functions from test.lua");
    //            return;
    //        }

    //        s_TestInitialized = true;
    //        LOGI("[ScriptDebug] Test script loaded successfully");
    //    }
    //    catch (const sol::error& e) {
    //        LOGE("[ScriptDebug] Failed to load test script: %s", e.what());
    //    }
    //}

    void ScriptDebug::Initialize() {
        LOGI("[ScriptDebug] Lua script testing enabled. Press K/L to run test script.");
        try {
            sol::state& lua = ScriptSystem::GetLuaState();
            //auto& AM = EngineFramework::AssetManagerSystem::GetInstance();

            // Lookup the script by name through AssetManager (no hardcoded file path)
            auto full_guid = EngineFramework::AssetManagerSystem::GetFullGuid(
                EngineFramework::AssetManagerSystem::SCRIPT_GUID,
                "test");  // match your script name (test.lua -> "test")

            auto scriptAsset = EngineFramework::AssetManagerSystem::GetResource<ScriptAsset>(full_guid);
            if (!scriptAsset) {
                LOGE("[ScriptDebug] Failed to find ScriptAsset named 'test' in AssetManager.");
                return;
            }

            // Load Lua chunk from the preloaded asset code
			auto chunk = lua.load(scriptAsset->code, "test.lua"); 
            //2nd Param here is the chunk name for error reporting
            //Can be changed to a string variable in the future with property manager
            chunk(); // execute

            // Cache function references
            s_GreetFunc = lua["greet"];
            s_AddFunc = lua["add"];

            if (!s_GreetFunc.valid() || !s_AddFunc.valid()) {
                LOGE("[ScriptDebug] Failed to load test functions from ScriptAsset 'test'");
                return;
            }

            s_TestInitialized = true;
            LOGI("[ScriptDebug] Test ScriptAsset loaded successfully");
        }
        catch (const sol::error& e) {
            LOGE("[ScriptDebug] Failed to load test script: %s", e.what());
        }
    }

    void ScriptDebug::Update(float dt) {
        (void)dt;
        if (!s_TestInitialized) return;

        // Press K to test Lua functions
        if (EngineFramework::InputManager::GetKeyDown(GLFW_KEY_K)) {
            LOGI("[ScriptDebug] K pressed - Testing Lua functions");

            try {
                // Test greet function
                if (s_GreetFunc.valid()) {
                    std::string result = s_GreetFunc("Developer");
                    LOGI("[ScriptDebug] Greet returned: %s", result.c_str());
                }
                LOGI("[ScriptDebug] Test complete");
            }
            catch (const sol::error& e) {
                LOGE("[ScriptDebug] Lua error: %s", e.what());
            }
        }
        // Press L to test Lua functions
        if (EngineFramework::InputManager::GetKeyDown(GLFW_KEY_L)) {
            LOGI("[ScriptDebug] L pressed - Testing Lua functions");

            try {
                // Test add function
                if (s_AddFunc.valid()) {
                    int sum = s_AddFunc(35, 32);
                    LOGI("[ScriptDebug] Add returned: %d", sum);
                }

                LOGI("[ScriptDebug] Test complete");
            }
            catch (const sol::error& e) {
                LOGE("[ScriptDebug] Lua error: %s", e.what());
            }
        }
    }

    void ScriptDebug::Shutdown() {
        s_GreetFunc = sol::nil;
        s_AddFunc = sol::nil;
        s_TestInitialized = false;
    }
}
#endif // ENGINE_ENABLE_LUA && DEBUG