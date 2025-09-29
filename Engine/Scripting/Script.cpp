/*
*****************************************************************/
/*!
\file   Script.cpp
\author Teng Jun Rong Jerron
\par    t.junrongjerron\@digipen.edu
\date   September 2025
\brief
This file contains the definitions of the functions that are needed for
handling the Scripts.
Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* *******************************************************************/

#include "Script.hpp"
#include <fstream>
#include <sstream>
#include <iostream>

namespace Engine {

#if defined(ENGINE_ENABLE_LUA)
    namespace { sol::state s_Lua; }
    sol::state& ScriptSystem::GetLuaState() { return s_Lua; }
#endif

    void ScriptSystem::Init() {
#if defined(ENGINE_ENABLE_LUA)
        s_Lua.open_libraries(sol::lib::base, sol::lib::package, sol::lib::math,
            sol::lib::table, sol::lib::string, sol::lib::io);
#endif
    }

    void ScriptSystem::Shutdown() {}

    // ---------------- Runtime start/stop ----------------
    void ScriptSystem::OnRuntimeStart(entt::registry& reg) {
        reg.view<LuaScriptComponent>().each([](auto& lsc) { lsc.Loaded = false; });
    }

    void ScriptSystem::OnRuntimeStop(entt::registry& reg) {
        auto nativeView = reg.view<NativeScriptComponent>();
        for (auto [e, nsc] : nativeView.each()) {
            if (nsc.Instance) { nsc.Instance->OnDestroy(); DestroyNativeInstance(nsc); }
        }
#if defined(ENGINE_ENABLE_LUA)
        auto luaView = reg.view<LuaScriptComponent>();
        for (auto [e, lsc] : luaView.each()) { UnloadLuaFor(lsc, /*callOnDestroy*/true); }
#endif
    }

    // ---------------- Per-frame update ----------------
    void ScriptSystem::OnUpdate(entt::registry& reg, float dt) {
        {   // Native
            auto view = reg.view<NativeScriptComponent>();
            for (auto [e, nsc] : view.each()) {
                if (!nsc.Enabled) continue;
                EnsureNativeInstance(reg, e, nsc);
                if (nsc.Instance) nsc.Instance->OnUpdate(dt);
            }
        }
#if defined(ENGINE_ENABLE_LUA)
        {   // Lua
            auto view = reg.view<LuaScriptComponent>();
            for (auto [e, lsc] : view.each()) {
                if (!lsc.Enabled) continue;
                if (!lsc.Loaded) LoadLuaFor(reg, e, lsc);
                if (lsc.Loaded && lsc.FnOnUpdate.valid()) {
                    try { lsc.FnOnUpdate(lsc.Self, dt); }
                    catch (const sol::error& err) {
                        std::cerr << "[Lua] on_update error in " << lsc.ScriptPath.string()
                            << " : " << err.what() << "\n";
                    }
                }
            }
        }
#endif
    }

    // ---------------- Helpers: Native ----------------
    void ScriptSystem::EnsureNativeInstance(entt::registry& reg,
        entt::entity e,
        NativeScriptComponent& nsc) {
        if (!nsc.Instance && nsc.Instantiate) {
            nsc.Instance = nsc.Instantiate();
            nsc.Instance->SetEntity(e, &reg);
            nsc.Instance->OnCreate();
        }
    }

    void ScriptSystem::DestroyNativeInstance(NativeScriptComponent& nsc) {
        if (nsc.Instance && nsc.Destroy) nsc.Destroy(nsc);
    }

    // ---------------- Helpers: Lua (optional) ----------------
#if defined(ENGINE_ENABLE_LUA)
    static std::string ReadTextFile(const std::filesystem::path& p) {
        std::ifstream ifs(p, std::ios::in | std::ios::binary);
        if (!ifs) return {};
        std::ostringstream ss; ss << ifs.rdbuf(); return ss.str();
    }

    void ScriptSystem::LoadLuaFor(entt::registry& reg,
        entt::entity /*e*/,
        LuaScriptComponent& lsc) {
		(void)reg; // Unused
        UnloadLuaFor(lsc, /*callOnDestroy*/false);

        if (lsc.ScriptPath.empty()) { lsc.Loaded = false; return; }

        // DEBUG: Print the full path being attempted
        std::filesystem::path fullPath = std::filesystem::absolute(lsc.ScriptPath);
        std::cerr << "[Lua] Attempting to read: " << fullPath.string() << "\n";
        std::cerr << "[Lua] File exists: " << std::filesystem::exists(lsc.ScriptPath) << "\n";

        const std::string code = ReadTextFile(lsc.ScriptPath);
        if (code.empty()) {
            std::cerr << "[Lua] Failed to read " << lsc.ScriptPath.string() << "\n";
            lsc.Loaded = false; return;
        }

        try {
            // Create environment for this script
            lsc.Env = sol::environment(s_Lua, sol::create, s_Lua.globals());

            // Load the script with the environment
            auto result = s_Lua.script(code, lsc.Env);

            if (!result.valid()) {
                sol::error err = result;
                std::cerr << "[Lua] Exec error in " << lsc.ScriptPath.string()
                    << " : " << err.what() << "\n";
                lsc.Loaded = false; return;
            }

            lsc.Self = sol::table(s_Lua, sol::create);
            lsc.FnOnCreate = lsc.Env["on_create"];
            lsc.FnOnUpdate = lsc.Env["on_update"];
            lsc.FnOnDestroy = lsc.Env["on_destroy"];
            lsc.Loaded = true;

            if (lsc.FnOnCreate.valid()) {
                try { lsc.FnOnCreate(lsc.Self); }
                catch (const sol::error& err) {
                    std::cerr << "[Lua] on_create error in " << lsc.ScriptPath.string()
                        << " : " << err.what() << "\n";
                }
            }
        }
        catch (const sol::error& err) {
            std::cerr << "[Lua] Unexpected error in " << lsc.ScriptPath.string()
                << " : " << err.what() << "\n";
            lsc.Loaded = false;
        }
    }

    void ScriptSystem::UnloadLuaFor(LuaScriptComponent& lsc, bool callOnDestroy) {
        if (lsc.Loaded && callOnDestroy && lsc.FnOnDestroy.valid()) {
            try { lsc.FnOnDestroy(lsc.Self); }
            catch (const sol::error& err) {
                std::cerr << "[Lua] on_destroy error in " << lsc.ScriptPath.string()
                    << " : " << err.what() << "\n";
            }
        }
        lsc.FnOnCreate = sol::nil;
        lsc.FnOnUpdate = sol::nil;
        lsc.FnOnDestroy = sol::nil;
        lsc.Self = sol::nil;
        lsc.Env = sol::nil;
        lsc.Loaded = false;
    }
#endif // ENGINE_ENABLE_LUA

    // ---------------- Hot reload ----------------
    void ScriptSystem::ReloadScript(entt::registry& reg, entt::entity e) {
#if defined(ENGINE_ENABLE_LUA)
        if (reg.any_of<LuaScriptComponent>(e)) {
            auto& lsc = reg.get<LuaScriptComponent>(e);
            LoadLuaFor(reg, e, lsc);
        }
#else
        (void)reg; (void)e;
#endif
    }

} // namespace Engine
