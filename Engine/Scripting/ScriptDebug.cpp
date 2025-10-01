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

#if defined(ENGINE_ENABLE_LUA) && defined(DEBUG)
#include "Script.hpp"
#include "InputManager.hpp"
#include <sol/sol.hpp>

namespace Engine {

    static bool s_TestInitialized = false;

    void ScriptDebug::Initialize() {
        LOGI("[ScriptDebug] Lua script testing enabled. Press L to run test script.");
        s_TestInitialized = true;
    }

    void ScriptDebug::Update(float dt) {
        (void)dt;

        if (!s_TestInitialized) return;

        // Press L to test Lua functions
        if (EngineFramework::InputManager::GetKeyDown(GLFW_KEY_L)) {
            LOGI("[ScriptDebug] L pressed - Testing Lua functions");

            try {
                sol::state& lua = ScriptSystem::GetLuaState();

                // Load the test script
                lua.script_file("Extern/Scripts/test.lua");

                // Test greet function
                sol::function greet = lua["greet"];
                if (greet.valid()) {
                    std::string result = greet("Developer");
                    LOGI("[ScriptDebug] Greet returned: %s", result.c_str());
                }

                // Test add function
                sol::function add = lua["add"];
                if (add.valid()) {
                    int sum = add(42, 13);
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
        s_TestInitialized = false;
    }
}

#endif // ENGINE_ENABLE_LUA && DEBUG