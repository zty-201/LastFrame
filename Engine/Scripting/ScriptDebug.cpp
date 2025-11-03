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

    /*
	    //How to use ScriptComponent to add scripts to an entity
        LuaScriptComponent lsc;
        lsc.Scripts.resize(2);
        lsc.Scripts[0].ScriptGuid = EngineFramework::AssetManagerSystem::GetFullGuid(
            EngineFramework::AssetManagerSystem::SCRIPT_GUID, "PlayerMovement").m_Instance;
        lsc.Scripts[1].ScriptGuid = EngineFramework::AssetManagerSystem::GetFullGuid(
            EngineFramework::AssetManagerSystem::SCRIPT_GUID, "HealthSystem").m_Instance;

        registry.emplace<LuaScriptComponent>(entity, std::move(lsc));
    */
    void ScriptDebug::Initialize() {
        LOGI("[ScriptDebug] Lua script testing enabled. Press K/L to run test scripts.");
        try {
            sol::state& lua = ScriptSystem::GetLuaState();

            // Load the test script once during initialization
            auto& AM = EngineFramework::AssetManagerSystem::GetInstance();
            std::string code = AM.GetStringFromFile("Extern/Scripts/test.lua");
            auto chunk = lua.load(code, "Extern/Scripts/test.lua");
            chunk(); // execute
            auto LoadAndRunScript = [&](const std::string& scriptName, sol::function& greetOut, sol::function& addOut) {
                auto full_guid = EngineFramework::AssetManagerSystem::GetFullGuid(
                    EngineFramework::AssetManagerSystem::SCRIPT_GUID,
                    scriptName);

                auto scriptAsset = EngineFramework::AssetManagerSystem::GetResource<ScriptAsset>(full_guid);
                if (!scriptAsset) {
                    LOGE("[ScriptDebug] Failed to find ScriptAsset '%s'", scriptName.c_str());
                    return false;
                }

                // Create isolated environment for each script
                sol::environment env(lua, sol::create, lua.globals());
                auto chunk = lua.load(scriptAsset->code, scriptName + ".lua");
                chunk(env); // execute within its environment

                // Extract test functions from that environment
                greetOut = env["greet"];
                addOut = env["add2"];

                if (!greetOut.valid() || !addOut.valid()) {
                    LOGE("[ScriptDebug] Script '%s' missing greet/add functions", scriptName.c_str());
                    return false;
                }

                LOGI("[ScriptDebug] Loaded ScriptAsset '%s' successfully", scriptName.c_str());
                return true;
                };

            bool ok1 = LoadAndRunScript("test", s_GreetFunc, s_AddFunc);
            bool ok2 = LoadAndRunScript("test2", s_GreetFunc, s_AddFunc); // load second script

            if (!ok1 && !ok2) {
                LOGE("[ScriptDebug] No scripts loaded successfully.");
                return;
            }

            s_TestInitialized = true;
            LOGI("[ScriptDebug] Both test scripts initialized.");
        }
        catch (const sol::error& e) {
            LOGE("[ScriptDebug] Failed to load test scripts: %s", e.what());
        }
    }

    void ScriptDebug::Update(float dt) {
        (void)dt;
        if (!s_TestInitialized) return;

        ////if (EngineFramework::InputManager::GetKeyDown(GLFW_KEY_K)) {
        //if (EngineFramework::InputManager::GetMouseButtonDown(0)) {
        //    LOGI("[ScriptDebug] K pressed - Running greet() from test.lua");
        //    try {
        //        if (s_GreetFunc.valid()) {
        //            std::string result = s_GreetFunc("Developer from test.lua");
        //            LOGI("[ScriptDebug] test.lua greet() returned: %s", result.c_str());
        //        }
        //    }
        //    catch (const sol::error& e) {
        //        LOGE("[ScriptDebug] Lua error in test.lua: %s", e.what());
        //    }
        //}

        //if (EngineFramework::InputManager::GetKeyDown(GLFW_KEY_L)) {
        //    LOGI("[ScriptDebug] L pressed - Running add() from test2.lua");
        //    try {
        //        if (s_AddFunc.valid()) {
        //            int sum = s_AddFunc(21, 79);
        //            LOGI("[ScriptDebug] test2.lua add() returned: %d", sum);
        //        }
        //    }
        //    catch (const sol::error& e) {
        //        LOGE("[ScriptDebug] Lua error in test2.lua: %s", e.what());
        //    }
        //}
    }

    void ScriptDebug::Shutdown() {
        s_GreetFunc = sol::nil;
        s_AddFunc = sol::nil;
        s_TestInitialized = false;
    }
}
#endif // ENGINE_ENABLE_LUA && DEBUG