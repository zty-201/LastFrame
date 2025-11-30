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

//For UI access in scripts
#include "UIComponent.h"

//For UI access in scripts
#include "PhysicsComponent.h"
#include "PhysicsWorld.hpp"

#include "AudioComponent.h"

//For platform android vs windows
#include "Platform.h"

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

        // ===============================================================
        // Create SHARED table for cross-script communication
        // ===============================================================
        s_Lua["SHARED"] = s_Lua.create_table();
        LOGI("[ScriptSystem] Created SHARED table for cross-script communication");

        // ===============================================================
        // Platform constant exposed to Lua
        // ===============================================================
#if defined(PLATFORM_ANDROID)
        s_Lua["PLATFORM"] = "android";
#elif defined(PLATFORM_WINDOWS)
        s_Lua["PLATFORM"] = "windows";
#else
        s_Lua["PLATFORM"] = "unknown";
#endif
        LOGI("[ScriptSystem] PLATFORM exposed to Lua: %s", s_Lua["PLATFORM"].get<std::string>().c_str());


        BindLuaPrintToEngine(s_Lua);

        LOGI("[ScriptSystem] Initializing Lua bindings...");

        // ===============================================================
        // Transform Component bindings
        // ===============================================================
        s_Lua.new_usertype<TransformComponent>("TransformComponent",
            "GetPosition", [](TransformComponent& transform) {
                return std::make_tuple(transform.position.x, transform.position.y, transform.position.z);
            },
            "SetPosition", [](TransformComponent& transform, float x, float y, float z) {
                transform.position = glm::vec3(x, y, z);
            },
            "GetRotation", [](TransformComponent& transform) {
                return std::make_tuple(transform.rotation.x, transform.rotation.y, transform.rotation.z);
            },
            "SetRotation", [](TransformComponent& transform, float x, float y, float z) {
                transform.rotation = glm::vec3(x, y, z);
            },
            "GetScale", [](TransformComponent& transform) {
                return std::make_tuple(transform.scale.x, transform.scale.y, transform.scale.z);
            },
            "SetScale", [](TransformComponent& transform, float x, float y, float z) {
                transform.scale = glm::vec3(x, y, z);
            }
        );

        LOGI("[ScriptSystem] TransformComponent bindings complete.");

        // ===============================================================
        // PhysicsComponent bindings
        // ===============================================================
        s_Lua.new_usertype<PhysicsComponent>("PhysicsComponent",
            "GetVelocity", [](PhysicsComponent& phys) {
                return std::make_tuple(
                    phys.velocity.x,
                    phys.velocity.y,
                    phys.velocity.z
                );
            },
            "SetVelocity", [](PhysicsComponent& phys, float x, float y, float z) {
                phys.velocity = glm::vec3(x, y, z);
            },
            "GetUseGravity", [](PhysicsComponent& phys) {
                return phys.useGravity;
            },
            "SetUseGravity", [](PhysicsComponent& phys, bool v) {
                phys.useGravity = v;
            },
            "GetGravityFactor", [](PhysicsComponent& phys) {
                return phys.gravityFactor;
            },
            "SetGravityFactor", [](PhysicsComponent& phys, float f) {
                phys.gravityFactor = f;
            },
            "SetLockRotation", [](PhysicsComponent& phys, bool v) {
                phys.lockRotation = v;
            }
        );

        // ===============================================================
        // Direct physics helpers (operate on the actual Jolt body)
        // ===============================================================
        s_Lua.set_function("Physics_SetVelocity",
            [](uint32_t entityId, float x, float y, float z)
            {
                entt::entity e = static_cast<entt::entity>(entityId);
                if (!PhysicsWorld::Get().HasBody(e))
                    return;

                glm::vec3 v{ x, y, z };
                PhysicsWorld::Get().SetLinearVelocity(e, v);
                PhysicsWorld::Get().ActivateBody(e);
            });

        s_Lua.set_function("Physics_GetVelocity",
            [](uint32_t entityId)
            {
                entt::entity e = static_cast<entt::entity>(entityId);
                if (!PhysicsWorld::Get().HasBody(e))
                    return std::make_tuple(0.0f, 0.0f, 0.0f);

                glm::vec3 v = PhysicsWorld::Get().GetBodyVelocity(e);
                return std::make_tuple(v.x, v.y, v.z);
            });

        // ===============================================================
        // Light Component bindings
        // ===============================================================
        // (Assuming your LightComponent has fields:
        //  glm::vec3 position, direction, color;
        //  float intensity, radius, constant, linear, quadratic,
        //        innerConeAngle, outerConeAngle)
        s_Lua.new_usertype<LightComponent>("LightComponent",
            "GetPosition", [](LightComponent& light) {
                return std::make_tuple(light.position.x, light.position.y, light.position.z);
            },
            "SetPosition", [](LightComponent& light, float x, float y, float z) {
                light.position = glm::vec3(x, y, z);
            },
            "GetDirection", [](LightComponent& light) {
                return std::make_tuple(light.direction.x, light.direction.y, light.direction.z);
            },
            "SetDirection", [](LightComponent& light, float x, float y, float z) {
                light.direction = glm::normalize(glm::vec3(x, y, z));
            },
            "GetIntensity", [](LightComponent& light) {
                return light.intensity;
            },
            "SetIntensity", [](LightComponent& light, float i) {
                light.intensity = i;
            }
        );

        LOGI("[ScriptSystem] LightComponent bindings complete.");

        // ===============================================================
        // AudioComponent bindings
        // ===============================================================
        s_Lua.new_usertype<AudioComponent>("AudioComponent",
            "GetVolume", [](AudioComponent& a) { return a.volume; },
            "SetVolume", [](AudioComponent& a, float v) { a.volume = v; },

            "GetLoop", [](AudioComponent& a) { return a.loop; },
            "SetLoop", [](AudioComponent& a, bool v) { a.loop = v; },

            "GetPlayOnStart", [](AudioComponent& a) { return a.playOnStart; },
            "SetPlayOnStart", [](AudioComponent& a, bool v) { a.playOnStart = v; },

            "GetIs3D", [](AudioComponent& a) { return a.is3D; },
            "SetIs3D", [](AudioComponent& a, bool v) { a.is3D = v; },

            "SetMinDistance", [](AudioComponent& a, float d) { a.minDistance = d; },
            "SetMaxDistance", [](AudioComponent& a, float d) { a.maxDistance = d; },

            "RequestPlay", [](AudioComponent& a) { a.playRequested = true; }
        );

        // ===============================================================
        // UIComponent bindings
        // ===============================================================
        s_Lua.new_usertype<UIComponent>("UIComponent",
            "GetPosition", [](UIComponent& ui) {
                return std::make_tuple(ui.position.x, ui.position.y);
            },
            "SetPosition", [](UIComponent& ui, float x, float y) {
                ui.position = glm::vec2(x, y);
            },
            "GetSize", [](UIComponent& ui) {
                return std::make_tuple(ui.size.x, ui.size.y);
            },
            "SetSize", [](UIComponent& ui, float w, float h) {
                ui.size = glm::vec2(w, h);
            },
            "GetColor", [](UIComponent& ui) {
                return std::make_tuple(ui.color.r, ui.color.g, ui.color.b);
            },
            "SetColor", [](UIComponent& ui, float r, float g, float b, float a) {
                ui.color = glm::vec4(r, g, b, a);
            },
            "IsVisible", [](UIComponent& ui) {
                return ui.visible;
            },
            "SetVisible", [](UIComponent& ui, bool v) {
                ui.visible = v;
            },
            "GetLayer", [](UIComponent& ui) {
                return ui.layer;
            },
            "SetLayer", [](UIComponent& ui, int layer) {
                ui.layer = layer;
            }
        );

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
        inputTable.set_function("GetGyroscopeRotatioRate", []() {
            glm::vec3 rate = EngineFramework::InputManager::GetGyroscopeRotatioRate();
            return std::make_tuple(rate.x, rate.y, rate.z);
            });

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

        // ===============================================================
        // Global helper: GetEntityTransform(entity)
        // ===============================================================
        s_Lua.set_function("GetEntityTransform", [](entt::entity e) -> TransformComponent* {
            if (registry.all_of<TransformComponent>(e)) {
                return &registry.get<TransformComponent>(e);
            }
            return nullptr;
            });

        // ===============================================================
        // Global helper: GetEntityUI(entity)
        // ===============================================================
        s_Lua.set_function("GetEntityUI", [](entt::entity e) -> UIComponent* {
            if (registry.all_of<UIComponent>(e)) {
                return &registry.get<UIComponent>(e);
            }
            return nullptr;
            });

        // ===============================================================
        // Global helper: GetEntityAudio(entity)
        // ===============================================================
        s_Lua.set_function("GetEntityAudio", [](entt::entity e) -> AudioComponent* {
            if (registry.all_of<AudioComponent>(e))
                return &registry.get<AudioComponent>(e);
            return nullptr;
            });



        // ===============================================================
        // GLFW Key Constants (for WASD and other keys)
        // ===============================================================
        s_Lua["GLFW_KEY_W"] = 87;
        s_Lua["GLFW_KEY_A"] = 65;
        s_Lua["GLFW_KEY_S"] = 83;
        s_Lua["GLFW_KEY_D"] = 68;

        s_Lua["GLFW_KEY_F"] = 70;
        s_Lua["GLFW_KEY_Q"] = 81;
        s_Lua["GLFW_KEY_E"] = 69;
        s_Lua["GLFW_KEY_C"] = 67;  // <-- add this line for calibrate

        s_Lua["GLFW_KEY_SPACE"] = 32;
        s_Lua["GLFW_KEY_SHIFT"] = 340;

        // Mouse button constants
        s_Lua["GLFW_MOUSE_BUTTON_LEFT"]   = 0;
        s_Lua["GLFW_MOUSE_BUTTON_RIGHT"]  = 1;
        s_Lua["GLFW_MOUSE_BUTTON_MIDDLE"] = 2;


        LOGI("[ScriptSystem] Lua bindings initialized successfully.");

        //No longer needed to sanity check here (Debugging)
        //// Quick sanity check that camera_touch_control.lua is loaded
        //auto guid = EngineFramework::AssetManagerSystem::GetFullGuid(
        //    EngineFramework::AssetManagerSystem::SCRIPT_GUID, "camera_touch_control");
        //LOGI("[Check] camera_touch_control full GUID = %llu", guid.m_Instance.m_Value); //Error here

        //auto scriptAsset = EngineFramework::AssetManagerSystem::GetResource<ScriptAsset>(guid);
        //if (!scriptAsset)
        //    LOGE("[Check] ScriptAsset 'camera_touch_control' not found!");
        //else
        //    LOGI("[Check] ScriptAsset found. Code length: %zu", scriptAsset->code.size());
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

        // Load camera scripts first to establish POV table
        auto cameraView = reg.view<CameraComponent, LuaScriptComponent>();
        for (auto [entity, camera, scripts] : cameraView.each()) {
            for (auto& inst : scripts.Scripts) {
                LoadLuaFor(reg, entity, inst);
            }
        }

        // Then load other scripts
        auto otherView = reg.view<LuaScriptComponent>(entt::exclude<CameraComponent>);
        for (auto [entity, scripts] : otherView.each()) {
            for (auto& inst : scripts.Scripts) {
                LoadLuaFor(reg, entity, inst);
            }
        }
        //No longer auto attach camera script here
        //// Attach camera_touch_control.lua to main camera (for Android + Windows)
        //auto cameraView = reg.view<CameraComponent>();
        //if (!cameraView.empty()) {
        //    auto cameraEntity = cameraView.front();

        //    LOGI("[ScriptSystem] Attaching script to camera entity (%u) with registry %p",
        //        static_cast<unsigned>(cameraEntity), (void*)&reg);

        //    if (!reg.all_of<LuaScriptComponent>(cameraEntity)) {
        //        reg.emplace<LuaScriptComponent>(cameraEntity);
        //    }

        //    auto& lsc = reg.get<LuaScriptComponent>(cameraEntity);
        //    lsc.Enabled = true;

        //    Engine::LuaScriptComponent::LuaInstance inst;
        //    inst.ScriptGuid = EngineFramework::AssetManagerSystem::GetFullGuid(
        //        EngineFramework::AssetManagerSystem::SCRIPT_GUID,
        //        "player_pov_camera"
        //    ).m_Instance;

        //    bool alreadyAttached = false;
        //    for (auto& s : lsc.Scripts) {
        //        if (s.ScriptGuid == inst.ScriptGuid) {
        //            alreadyAttached = true;
        //            LOGE("[ScriptSystem] camera_touch_control already attached to camera, skipping reattach.");
        //            break;
        //        }
        //    }

        //    if (!alreadyAttached) {
        //        lsc.Scripts.push_back(inst);
        //        LOGI("[ScriptSystem] camera_touch_control successfully attached to camera entity (%u).",
        //            static_cast<unsigned>(cameraEntity));

        //        Engine::ScriptSystem::ReloadScript(reg, cameraEntity);
        //    }
        //}
        //else {
        //    LOGE("[ScriptSystem] No camera entity found! Skipping camera_touch_control attachment.");
        //}
#endif
    }

    void ScriptSystem::OnRuntimeStop(entt::registry& reg) {;
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

            // ===============================================================
            // Prepare entity-specific data BEFORE creating environment
            // ===============================================================
            TransformComponent* entityTransform = nullptr;
            if (reg.all_of<TransformComponent>(e)) {
                entityTransform = &reg.get<TransformComponent>(e);
                LOGI("[LuaRuntime] Entity %u has TransformComponent at %p",
                    static_cast<unsigned>(e), (void*)entityTransform);
            }
            else {
                LOGE("[LuaRuntime] Entity %u does NOT have TransformComponent!",
                    static_cast<unsigned>(e));
            }

            // Get LightComponent
            LightComponent* entityLight = nullptr;
            if (registry.all_of<LightComponent>(e)) {
                entityLight = &registry.get<LightComponent>(e);
            }

            // Get UIComponent
            UIComponent* entityUI = nullptr;
            if (registry.all_of<UIComponent>(e)) {
                entityUI = &registry.get<UIComponent>(e);
            }

            // Get PhysicsComponent
            PhysicsComponent* entityPhysics = nullptr;
            if (registry.all_of<PhysicsComponent>(e)) {
                entityPhysics = &registry.get<PhysicsComponent>(e);
            }

            // Get AudioComponent
            AudioComponent* entityAudio = nullptr;
            if (registry.all_of<AudioComponent>(e)) {
                entityAudio = &registry.get<AudioComponent>(e);
            }



            // Create environment and inject variables
            inst.Env = sol::environment(s_Lua, sol::create, s_Lua.globals());
            inst.Env["EntityID"] = static_cast<uint32_t>(e);
            inst.Env["EntityTransform"] = entityTransform;
            inst.Env["EntityLight"] = entityLight;
            inst.Env["EntityUI"] = entityUI;
            inst.Env["EntityPhysics"] = entityPhysics;
            inst.Env["EntityAudio"] = entityAudio;

            LOGI("[LuaRuntime] Injected EntityID=%u and EntityTransform=%p into environment",
                static_cast<unsigned>(e), (void*)entityTransform);

            std::string label = EngineFramework::AssetManagerSystem::GetName(full_guid);

            // Load and execute script with environment
            sol::protected_function scriptFunc = s_Lua.load(code, label);
            sol::set_environment(inst.Env, scriptFunc);

            auto result = scriptFunc();

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