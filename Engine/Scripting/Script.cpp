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

//For camera access in scripts
#include "InputManager.hpp"
#include "CameraComponent.h"
#include "Camera.h"

namespace Engine {

#if defined(ENGINE_ENABLE_LUA)
    namespace { sol::state s_Lua; }
    sol::state& ScriptSystem::GetLuaState() { return s_Lua; }
#endif

    // ===============================================================
    // New function: Hijack Lua's 'print' to use Engine's LOGI
    // ===============================================================
#if defined(ENGINE_ENABLE_LUA)
    // NOTE: If LOGI is not defined here, you may need to include 
    // an appropriate header (e.g., "Logging.hpp" or similar).
    void BindLuaPrintToEngine(sol::state& lua)
    {
        // Override the global 'print' function in Lua
        lua["print"] = [&](sol::variadic_args va)
            {
                std::string message;
                // Iterate through all arguments passed to Lua's print
                for (auto arg : va)
                {
                    // Convert the argument to a string in Lua
                    // We use the environment's tostring (or global if nil) to handle various types
                    std::string s = lua["tostring"](arg);

                    // Append the result to the message, adding a space separator
                    message += s + " ";
                }

                // Call your engine's C++ logging macro
                // Remove the trailing space if the message is not empty
                if (!message.empty()) {
                    message.pop_back(); // Remove the last space
                }
                LOGI("[LuaPrint] %s", message.c_str());
            };
    }
#endif
    // ===============================================================

    void ScriptSystem::Init() {
#if defined(ENGINE_ENABLE_LUA)
        s_Lua.open_libraries(sol::lib::base, sol::lib::package, sol::lib::math,
            sol::lib::table, sol::lib::string, sol::lib::io);

        BindLuaPrintToEngine(s_Lua);
        
        LOGI("[ScriptSystem] Initializing Lua bindings...");

        // ===============================================================
        // InputManager bindings
        // ===============================================================
        s_Lua.set_function("Input", []() -> EngineFramework::InputManager& {
            return EngineFramework::InputManager::GetInstance();
            });

        // First, bind the InputManager type with all its methods
        s_Lua.new_usertype<EngineFramework::InputManager>("InputManager",
            // Static method bindings
            "GetMouseButtonDown", &EngineFramework::InputManager::GetMouseButtonDown,
            "GetMouseButton", &EngineFramework::InputManager::GetMouseButton,
            "GetMouseButtonUp", &EngineFramework::InputManager::GetMouseButtonUp,
            "GetMousePosition", [](EngineFramework::InputManager&) {
                auto pos = EngineFramework::InputManager::GetMousePosition();
                return std::make_tuple(pos.first, pos.second);
            },
            "GetMouseDelta", [](EngineFramework::InputManager&) {
                auto delta = EngineFramework::InputManager::GetMouseDelta();
                return std::make_tuple(delta.first, delta.second);
            },
            "GetKey", sol::overload(
                static_cast<bool(*)(int)>(&EngineFramework::InputManager::GetKey),
                static_cast<bool(*)(EngineFramework::Key_Actions)>(&EngineFramework::InputManager::GetKey)
            ),
            "GetKeyDown", sol::overload(
                static_cast<bool(*)(int)>(&EngineFramework::InputManager::GetKeyDown),
                static_cast<bool(*)(EngineFramework::Key_Actions)>(&EngineFramework::InputManager::GetKeyDown)
            ),
            "GetKeyUp", sol::overload(
                static_cast<bool(*)(int)>(&EngineFramework::InputManager::GetKeyUp),
                static_cast<bool(*)(EngineFramework::Key_Actions)>(&EngineFramework::InputManager::GetKeyUp)
            )
        );

        // Create a global Input table/namespace with direct function access
        sol::table inputTable = s_Lua.create_named_table("Input");

        // Bind static methods directly to the Input table
        inputTable.set_function("GetMouseButtonDown", &EngineFramework::InputManager::GetMouseButtonDown);
        inputTable.set_function("GetMouseButton", &EngineFramework::InputManager::GetMouseButton);
        inputTable.set_function("GetMouseButtonUp", &EngineFramework::InputManager::GetMouseButtonUp);
        inputTable.set_function("GetMousePosition", []() {
            auto pos = EngineFramework::InputManager::GetMousePosition();
            return std::make_tuple(pos.first, pos.second);
            });
        inputTable.set_function("GetMouseDelta", []() {
            auto delta = EngineFramework::InputManager::GetMouseDelta();
            return std::make_tuple(delta.first, delta.second);
            });
        inputTable.set_function("GetKey", sol::overload(
            static_cast<bool(*)(int)>(&EngineFramework::InputManager::GetKey),
            static_cast<bool(*)(EngineFramework::Key_Actions)>(&EngineFramework::InputManager::GetKey)
        ));
        inputTable.set_function("GetKeyDown", sol::overload(
            static_cast<bool(*)(int)>(&EngineFramework::InputManager::GetKeyDown),
            static_cast<bool(*)(EngineFramework::Key_Actions)>(&EngineFramework::InputManager::GetKeyDown)
        ));
        inputTable.set_function("GetKeyUp", sol::overload(
            static_cast<bool(*)(int)>(&EngineFramework::InputManager::GetKeyUp),
            static_cast<bool(*)(EngineFramework::Key_Actions)>(&EngineFramework::InputManager::GetKeyUp)
        ));

        LOGI("[ScriptSystem] InputManager bindings complete (static table approach)");
        // ===============================================================
        // CameraComponent bindings //alot of extras lol
        // ===============================================================
        s_Lua.new_usertype<CameraComponent>("CameraComponent",
            "GetPosition", [](CameraComponent& cam) {
                return std::make_tuple(cam.position.x, cam.position.y, cam.position.z);
            },
            "SetPosition", [](CameraComponent& cam, float x, float y, float z) {
                cam.position = glm::vec3(x, y, z);
            },
            "GetTarget", [](CameraComponent& cam) {
                return std::make_tuple(cam.target.x, cam.target.y, cam.target.z);
            },
            "SetTarget", [](CameraComponent& cam, float x, float y, float z) {
                cam.target = glm::vec3(x, y, z);
            },
            "GetFOV", [](CameraComponent& cam) { return cam.fieldOfView; },
            "SetFOV", [](CameraComponent& cam, float fov) { cam.fieldOfView = fov; },
            "GetNearPlane", [](CameraComponent& cam) { return cam.nearPlane; },
            "SetNearPlane", [](CameraComponent& cam, float np) { cam.nearPlane = np; },
            "GetFarPlane", [](CameraComponent& cam) { return cam.farPlane; },
            "SetFarPlane", [](CameraComponent& cam, float fp) { cam.farPlane = fp; },
            "SetType", [](CameraComponent& cam, int type) {
                cam.camType = static_cast<CameraComponent::CameraType>(type);
            },
            "LookAt", [](CameraComponent& cam, float tx, float ty, float tz) {
                cam.target = glm::vec3(tx, ty, tz);
                cam.view_vector = glm::lookAt(cam.position, cam.target, glm::vec3(0, 1, 0));
            }
        );

        // ===============================================================
        // Camera namespace bindings
        // ===============================================================
        sol::table cameraTable = s_Lua.create_named_table("Camera");
        cameraTable.set_function("onCursor", &Camera::onCursor);
        cameraTable.set_function("onScroll", &Camera::onScroll);
        cameraTable.set_function("setType", &Camera::setType);
        cameraTable.set_function("Move", &Camera::Move_Cam_and_Target);
        cameraTable.set_function("getLookAt", &Camera::getLookAt);
        cameraTable.set_function("getPerspective", &Camera::getPerspective);

        // ===============================================================
        // Global helper: GetMainCamera()
        // ===============================================================
        s_Lua.set_function("GetMainCamera", []() -> CameraComponent* {
            auto view = registry.view<CameraComponent>();
            if (!view.empty()) {
                return &view.get<CameraComponent>(view.front());
            }
            return nullptr;
            });

        LOGI("[ScriptSystem] Lua bindings initialized successfully.");

        // Quick sanity check that camera_touch_control.lua is loaded
        auto guid = EngineFramework::AssetManagerSystem::GetFullGuid(
            EngineFramework::AssetManagerSystem::SCRIPT_GUID, "camera_touch_control");
        LOGI("[Check] camera_touch_control full GUID = %llu", guid.m_Instance.m_Value);

        auto scriptAsset = EngineFramework::AssetManagerSystem::GetResource<ScriptAsset>(guid);
        if (!scriptAsset)
            LOGE("[Check] ScriptAsset 'camera_touch_control' not found!");
        else
            LOGI("[Check] ScriptAsset found. Code length: %zu", scriptAsset->code.size());
#endif
    }

    void ScriptSystem::Shutdown() {}

    // ------------------------------------------------------------------
    // Runtime start/stop
    // ------------------------------------------------------------------
    void ScriptSystem::OnRuntimeStart(entt::registry& reg) {
#if defined(ENGINE_ENABLE_LUA)
        LOGI("[ScriptSystem] OnRuntimeStart called — initializing Lua scripts...");

        // Reset load state for all LuaScriptComponents
        reg.view<LuaScriptComponent>().each([](auto& comp) {
            for (auto& inst : comp.Scripts)
                inst.Loaded = false;
            });

        // Attach camera_touch_control.lua to main camera (for Android + Windows)
        auto cameraView = reg.view<CameraComponent>();
        if (!cameraView.empty()) {
            auto cameraEntity = cameraView.front();

            LOGI("[ScriptSystem] Attaching script to camera entity (%u) with registry %p",
                static_cast<unsigned>(cameraEntity), (void*)&reg);

            if (!reg.all_of<LuaScriptComponent>(cameraEntity)) {
                reg.emplace<LuaScriptComponent>(cameraEntity);
            }

            auto& lsc = reg.get<LuaScriptComponent>(cameraEntity);
            lsc.Enabled = true;

            Engine::LuaScriptComponent::LuaInstance inst;
            inst.ScriptGuid = EngineFramework::AssetManagerSystem::GetFullGuid(
                EngineFramework::AssetManagerSystem::SCRIPT_GUID,
                "player_pov_camera"
            ).m_Instance;

            bool alreadyAttached = false;
            for (auto& s : lsc.Scripts) {
                if (s.ScriptGuid == inst.ScriptGuid) {
                    alreadyAttached = true;
                    LOGE("[ScriptSystem] camera_touch_control already attached to camera, skipping reattach.");
                    break;
                }
            }

            if (!alreadyAttached) {
                lsc.Scripts.push_back(inst);
                LOGI("[ScriptSystem] camera_touch_control successfully attached to camera entity (%u).",
                    static_cast<unsigned>(cameraEntity));

                Engine::ScriptSystem::ReloadScript(reg, cameraEntity);
            }
        }
        else {
            LOGE("[ScriptSystem] No camera entity found! Skipping camera_touch_control attachment.");
        }
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

    // ------------------------------------------------------------------
    // Per-frame update
    // ------------------------------------------------------------------
    void ScriptSystem::OnUpdate(entt::registry& reg, float dt) {
        {   // Native scripts
            auto view = reg.view<NativeScriptComponent>();
            for (auto [e, nsc] : view.each()) { 
                LOGI("ScriptOnupdate is running");
                if (!nsc.Enabled) continue;
                EnsureNativeInstance(reg, e, nsc);
                if (nsc.Instance) nsc.Instance->OnUpdate(dt);
            }
        }

#if defined(ENGINE_ENABLE_LUA)
        {   // Lua scripts
            auto view = reg.view<LuaScriptComponent>();
            for (auto [e, comp] : view.each()) {
               //LOGI("[ScriptSystem] OnUpdate running with registry %p", (void*)&reg);
               //LOGI("[ScriptSystem] LuaScriptComponent.Enabled = %d for entity %u",
               //     comp.Enabled, static_cast<unsigned>(e));
                if (!comp.Enabled) continue;
                for (auto& inst : comp.Scripts) {
                    if (!inst.Loaded)
                        LoadLuaFor(reg, e, inst);

                    //if (inst.FnOnUpdate.valid()) {
                    //    LOGI("[LuaRuntime] FnOnUpdate is VALID for GUID %llu", inst.ScriptGuid.m_Value);
                    //}
                    //else {
                    //    LOGE("[LuaRuntime] FnOnUpdate is NIL for GUID %llu", inst.ScriptGuid.m_Value);
                    //}

                    if (inst.Loaded && inst.FnOnUpdate.valid()) { //Calling on_update for scripts side
                        try {
                            //LOGI("[LuaRuntime] on_update() for entity %u, script GUID %llu",
                            //    static_cast<unsigned>(e), inst.ScriptGuid.m_Value);
                            inst.FnOnUpdate(inst.Self, dt);
                        }
                        catch (const sol::error& err) {
                            LOGE("[LuaRuntime] on_update() error (GUID %llu): %s",
                                inst.ScriptGuid.m_Value, err.what());
                        }
                    }
                }
            }
        }
#endif
    }

    // ------------------------------------------------------------------
    // Native helpers
    // ------------------------------------------------------------------
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

    // ------------------------------------------------------------------
    // Lua helpers
    // ------------------------------------------------------------------
#if defined(ENGINE_ENABLE_LUA)

    //static std::string ReadTextFromAssets(const std::filesystem::path& relPath) {
    //    // relPath should be relative to your Assets root, e.g. "Extern/Scripts/hello.lua"
    //    auto& AM = EngineFramework::AssetManagerSystem::GetInstance();
    //    return AM.GetStringFromFile(relPath);
    //}

    void ScriptSystem::LoadLuaFor(entt::registry& reg,
        entt::entity e,
        LuaScriptComponent::LuaInstance& inst)
    {
        (void)reg;
        UnloadLuaFor(inst, /*callOnDestroy*/false);

        if (inst.ScriptGuid.m_Value == 0) {
            LOGE("[LuaRuntime] Invalid Script GUID; skipping load (entity %u)",
                static_cast<unsigned>(e));
            inst.Loaded = false;
            return;
        }

        auto full_guid = xresource::full_guid(
            inst.ScriptGuid,
            EngineFramework::AssetManagerSystem::SCRIPT_GUID);

        auto scriptAsset = EngineFramework::AssetManagerSystem::GetResource<ScriptAsset>(full_guid);
        if (!scriptAsset) {
            LOGE("[LuaRuntime] ScriptAsset not found for GUID %llu (entity %u)",
                inst.ScriptGuid.m_Value, static_cast<unsigned>(e));
            inst.Loaded = false;
            return;
        }

        const std::string& code = scriptAsset->code;
        if (code.empty()) {
            LOGE("[LuaRuntime] ScriptAsset empty for GUID %llu (entity %u)",
                inst.ScriptGuid.m_Value, static_cast<unsigned>(e));
            inst.Loaded = false;
            return;
        }

        try {
            full_guid = xresource::full_guid(
                inst.ScriptGuid,
                EngineFramework::AssetManagerSystem::SCRIPT_GUID
            );

            LOGI("[LuaRuntime] Loading script '%s' (GUID %llu) for entity %u",
                EngineFramework::AssetManagerSystem::GetName(full_guid).c_str(),
                inst.ScriptGuid.m_Value, static_cast<unsigned>(e));

            inst.Env = sol::environment(s_Lua, sol::create, s_Lua.globals());
            std::string label = EngineFramework::AssetManagerSystem::GetName(full_guid);
            auto result = s_Lua.load(code, label)(inst.Env);

            if (!result.valid()) {
                sol::error err = result;
                LOGE("[LuaRuntime] Execution error in %s: %s", label.c_str(), err.what());
                inst.Loaded = false;
                return;
            }

            inst.Self = sol::table(s_Lua, sol::create);
            inst.FnOnCreate = inst.Env["on_create"];
            inst.FnOnUpdate = inst.Env["on_update"];
            inst.FnOnDestroy = inst.Env["on_destroy"];
            inst.Loaded = true;

            if (inst.FnOnCreate.valid()) {
                LOGI("[LuaRuntime] Calling on_create() for script '%s' (entity %u, GUID %llu)",
                    label.c_str(), static_cast<unsigned>(e), inst.ScriptGuid.m_Value);
                try {
                    inst.FnOnCreate(inst.Self);
                    LOGI("[LuaRuntime] on_create() finished successfully for '%s'", label.c_str());
                }
                catch (const sol::error& err) {
                    LOGE("[LuaRuntime] on_create() error in %s: %s", label.c_str(), err.what());
                }
            }
            else {
                LOGE("[LuaRuntime] No on_create() found in script '%s'", label.c_str());
            }
        }
        catch (const sol::error& err) {
            LOGE("[LuaRuntime] Unexpected load error (GUID %llu): %s",
                inst.ScriptGuid.m_Value, err.what());
            inst.Loaded = false;
        }
    }

    void ScriptSystem::UnloadLuaFor(LuaScriptComponent::LuaInstance& inst, bool callOnDestroy) {
        if (inst.Loaded && callOnDestroy && inst.FnOnDestroy.valid()) {
            LOGI("[LuaRuntime] Calling on_destroy() for script GUID %llu", inst.ScriptGuid.m_Value);
            try {
                inst.FnOnDestroy(inst.Self);
                LOGI("[LuaRuntime] on_destroy() finished successfully (GUID %llu)", inst.ScriptGuid.m_Value);
            }
            catch (const sol::error& err) {
                LOGE("[LuaRuntime] on_destroy() error (GUID %llu): %s", inst.ScriptGuid.m_Value, err.what());
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

    // ------------------------------------------------------------------
    // Hot reload
    // ------------------------------------------------------------------
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
