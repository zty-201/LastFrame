#include "ScriptDebug.hpp"
#include "Scripting/ScriptAsset.hpp"

#if defined(ENGINE_ENABLE_LUA) && defined(DEBUG)
#include "Script.hpp"
#include "InputManager.hpp"
#include <sol/sol.hpp>

namespace Engine {

    static bool s_TestInitialized = false;
    static sol::function s_GreetFunc;
    static sol::function s_AddFunc;

    void ScriptDebug::Initialize() {
        //LOGI("[ScriptDebug] Lua script testing enabled. Press K/L to run test scripts.");
        //try {
        //    sol::state& lua = ScriptSystem::GetLuaState();

        //    auto LoadAndRunScript = [&](const std::string& scriptName,
        //        sol::function& greetOut,
        //        sol::function& addOut)
        //        {
        //            auto full_guid = EngineFramework::AssetManagerSystem::GetFullGuid(
        //                EngineFramework::AssetManagerSystem::SCRIPT_GUID,
        //                scriptName);

        //            auto& AM = EngineFramework::AssetManagerSystem::GetInstance();

        //            // Load the main test script
        //            std::string code = AM.GetStringFromFile("Extern/Scripts/test.lua");
        //            auto chunk_main = lua.load(code, "Extern/Scripts/test.lua");
        //            chunk_main(); // execute base test script

        //            // Load the target script asset
        //            auto scriptAsset = EngineFramework::AssetManagerSystem::GetResource<ScriptAsset>(full_guid);
        //            if (!scriptAsset) {
        //                LOGE("[ScriptDebug] Failed to find ScriptAsset '%s'", scriptName.c_str());
        //                return false;
        //            }

        //            // Create isolated environment for each script
        //            sol::environment env(lua, sol::create, lua.globals());
        //            auto chunk_env = lua.load(scriptAsset->code, scriptName + ".lua");
        //            chunk_env(env); // execute within its environment

        //            // Extract functions
        //            greetOut = env["greet"];
        //            addOut = env["add2"];

        //            if (!greetOut.valid() || !addOut.valid()) {
        //                LOGE("[ScriptDebug] Script '%s' missing greet/add functions", scriptName.c_str());
        //                return false;
        //            }

        //            LOGI("[ScriptDebug] Loaded ScriptAsset '%s' successfully", scriptName.c_str());
        //            return true;
        //        };

        //    bool ok1 = LoadAndRunScript("test", s_GreetFunc, s_AddFunc);
        //    bool ok2 = LoadAndRunScript("test2", s_GreetFunc, s_AddFunc);

        //    if (!ok1 && !ok2) {
        //        LOGE("[ScriptDebug] No scripts loaded successfully.");
        //        return;
        //    }

        //    s_TestInitialized = true;
        //    LOGI("[ScriptDebug] Both test scripts initialized.");
        //}
        //catch (const sol::error& e) {
        //    LOGE("[ScriptDebug] Failed to load test scripts: %s", e.what());
        //}
    }

    void ScriptDebug::Update(float dt) {
        (void)dt;
        if (!s_TestInitialized) return;
        // Currently no input-based script calls active
    }

    void ScriptDebug::Shutdown() {
        s_GreetFunc = sol::nil;
        s_AddFunc = sol::nil;
        s_TestInitialized = false;
    }

} // namespace Engine
#endif // ENGINE_ENABLE_LUA && DEBUG
