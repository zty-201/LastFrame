/*
*****************************************************************/
/*!
\file   Script.cpp
\author Teng Jun Rong Jerron
\par    t.junrongjerron@digipen.edu
\date   September 2025
\brief
Multi-script version: allows multiple Lua scripts per entity.
Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* *******************************************************************/

#include "Script.hpp"
#include "Scripting/ScriptAsset.hpp"
#include "AssetManager.hpp"
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
#if defined(ENGINE_ENABLE_LUA)
        reg.view<LuaScriptComponent>().each([](auto& comp) {
            for (auto& inst : comp.Scripts)
                inst.Loaded = false;
            });
#endif
    }

    void ScriptSystem::OnRuntimeStop(entt::registry& reg) {
        auto nativeView = reg.view<NativeScriptComponent>();
        for (auto [e, nsc] : nativeView.each()) {
            if (nsc.Instance) { nsc.Instance->OnDestroy(); DestroyNativeInstance(nsc); }
        }

#if defined(ENGINE_ENABLE_LUA)
        auto luaView = reg.view<LuaScriptComponent>();
        for (auto [e, comp] : luaView.each()) {
            for (auto& inst : comp.Scripts)
                UnloadLuaFor(inst, /*callOnDestroy*/true);
        }
#endif
    }

    // ---------------- Per-frame update ----------------
    void ScriptSystem::OnUpdate(entt::registry& reg, float dt) {
        {   // Native scripts
            auto view = reg.view<NativeScriptComponent>();
            for (auto [e, nsc] : view.each()) {
                if (!nsc.Enabled) continue;
                EnsureNativeInstance(reg, e, nsc);
                if (nsc.Instance) nsc.Instance->OnUpdate(dt);
            }
        }

#if defined(ENGINE_ENABLE_LUA)
        {   // Lua scripts
            auto view = reg.view<LuaScriptComponent>();
            for (auto [e, comp] : view.each()) {
                if (!comp.Enabled) continue;

                for (auto& inst : comp.Scripts) {
                    if (!inst.Loaded)
                        LoadLuaFor(reg, e, inst);

                    if (inst.Loaded && inst.FnOnUpdate.valid()) {
                        try { inst.FnOnUpdate(inst.Self, dt); }
                        catch (const sol::error& err) {
                            std::cerr << "[Lua] on_update error: " << err.what() << "\n";
                        }
                    }
                }
            }
        }
#endif
    }

    // ---------------- Helpers: Native ----------------
    void ScriptSystem::EnsureNativeInstance(entt::registry& reg,
        entt::entity e,
        NativeScriptComponent& nsc)
    {
        if (!nsc.Instance && nsc.Instantiate) {
            nsc.Instance = nsc.Instantiate();
            nsc.Instance->SetEntity(e, &reg);
            nsc.Instance->OnCreate();
        }
    }

    void ScriptSystem::DestroyNativeInstance(NativeScriptComponent& nsc) {
        if (nsc.Instance && nsc.Destroy) nsc.Destroy(nsc);
    }

    // ---------------- Helpers: Lua ----------------
#if defined(ENGINE_ENABLE_LUA)
    void ScriptSystem::LoadLuaFor(entt::registry& reg,
        entt::entity /*e*/,
        LuaScriptComponent::LuaInstance& inst)
    {
        (void)reg;
        UnloadLuaFor(inst, /*callOnDestroy*/false);

        if (inst.ScriptGuid.m_Value == 0) {
            std::cerr << "[Lua] Invalid Script GUID; skipping load.\n";
            inst.Loaded = false;
            return;
        }

        // Retrieve ScriptAsset
        auto full_guid = xresource::full_guid(
            inst.ScriptGuid,
            EngineFramework::AssetManagerSystem::SCRIPT_GUID);

        auto scriptAsset = EngineFramework::AssetManagerSystem::GetResource<ScriptAsset>(full_guid);
        if (!scriptAsset) {
            std::cerr << "[Lua] ScriptAsset not found for GUID "
                << inst.ScriptGuid.m_Value << "\n";
            inst.Loaded = false;
            return;
        }

        const std::string& code = scriptAsset->code;
        if (code.empty()) {
            std::cerr << "[Lua] ScriptAsset empty for GUID "
                << inst.ScriptGuid.m_Value << "\n";
            inst.Loaded = false;
            return;
        }

        try {
            // Create isolated environment for each script
            inst.Env = sol::environment(s_Lua, sol::create, s_Lua.globals());
            std::string label = EngineFramework::AssetManagerSystem::GetName(inst.ScriptGuid);
            auto result = s_Lua.load(code, label)(inst.Env);

            if (!result.valid()) {
                sol::error err = result;
                std::cerr << "[Lua] Exec error in " << label
                    << " : " << err.what() << "\n";
                inst.Loaded = false;
                return;
            }

            // Cache functions
            inst.Self = sol::table(s_Lua, sol::create);
            inst.FnOnCreate = inst.Env["on_create"];
            inst.FnOnUpdate = inst.Env["on_update"];
            inst.FnOnDestroy = inst.Env["on_destroy"];
            inst.Loaded = true;

            // Fire on_create if defined
            if (inst.FnOnCreate.valid()) {
                try { inst.FnOnCreate(inst.Self); }
                catch (const sol::error& err) {
                    std::cerr << "[Lua] on_create error in " << label
                        << " : " << err.what() << "\n";
                }
            }
        }
        catch (const sol::error& err) {
            std::cerr << "[Lua] Unexpected error while loading GUID "
                << inst.ScriptGuid.m_Value << " : " << err.what() << "\n";
            inst.Loaded = false;
        }
    }

    void ScriptSystem::UnloadLuaFor(LuaScriptComponent::LuaInstance& inst, bool callOnDestroy) {
        if (inst.Loaded && callOnDestroy && inst.FnOnDestroy.valid()) {
            try { inst.FnOnDestroy(inst.Self); }
            catch (const sol::error& err) {
                std::cerr << "[Lua] on_destroy error: " << err.what() << "\n";
            }
        }

        inst.FnOnCreate = sol::nil;
        inst.FnOnUpdate = sol::nil;
        inst.FnOnDestroy = sol::nil;
        inst.Self = sol::nil;
        inst.Env = sol::nil;
        inst.Loaded = false;
    }
#endif // ENGINE_ENABLE_LUA

    // ---------------- Hot reload ----------------
    void ScriptSystem::ReloadScript(entt::registry& reg, entt::entity e) {
#if defined(ENGINE_ENABLE_LUA)
        if (reg.any_of<LuaScriptComponent>(e)) {
            auto& comp = reg.get<LuaScriptComponent>(e);
            for (auto& inst : comp.Scripts)
                LoadLuaFor(reg, e, inst);
        }
#else
        (void)reg; (void)e;
#endif
    }

} // namespace Engine
