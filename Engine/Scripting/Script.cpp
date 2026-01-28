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
#include <glm/geometric.hpp> 
#include <iostream>

#if defined(ENGINE_ENABLE_LUA)
#include <mutex>
#include <unordered_set>
#endif

//For camera access in scripts
#include "InputManager.hpp"
#include "CameraComponent.h"
#include "Camera.h"

//For UI access in scripts
#include "UIComponent.h"

//For UI access in scripts
#include "PhysicsComponent.h"
#include "PhysicsWorld.hpp"

// For Audio
#include "AudioComponent.h"
#include "Audio.hpp"

//For platform android vs windows
#include "Platform.h"

namespace Engine {

#if defined(ENGINE_ENABLE_LUA)
    namespace {
        sol::state s_Lua;

        // ---------------------------------------------------------------
        // Hot reload request queue (safe point processing)
        // ---------------------------------------------------------------
        std::mutex s_ReloadMutex;
        bool s_ReloadAllRequested = false;
        std::unordered_set<entt::entity> s_ReloadEntityRequests;

        entt::registry* s_RuntimeRegistry = nullptr;
    }
    sol::state& ScriptSystem::GetLuaState() { return s_Lua; }
#endif

    // ===============================================================
    // New function: Hijack Lua's 'print' to use Engine's LOGI
    // ===============================================================
#if defined(ENGINE_ENABLE_LUA)
    void BindLuaPrintToEngine(sol::state& lua)
    {
        lua["print"] = [&](sol::variadic_args va)
            {
                std::string message;
                for (auto arg : va)
                {
                    std::string s = lua["tostring"](arg);
                    message += s + " ";
                }

                if (!message.empty()) {
                    message.pop_back();
                }
                LOGI("[LuaPrint] %s", message.c_str());
            };
    }
#endif
    // ===============================================================

    //void ScriptSystem::Init_Keyaction() {

    //    // ===============================================================
    //    // KeyActions bindings
    //    // ===============================================================

    //    sol::table keyActions = s_Lua.create_named_table("Key_Actions");

    //    keyActions["FLASHLIGHT"] = (int)EngineFramework::Key_Actions::FLASHLIGHT;
    //    keyActions["INTERACT"] = (int)EngineFramework::Key_Actions::INTERACT;
    //    keyActions["INVENTORY"] = (int)EngineFramework::Key_Actions::INVENTORY;
    //    keyActions["JUMP"] = (int)EngineFramework::Key_Actions::JUMP;
    //    keyActions["MOVE_BACK"] = (int)EngineFramework::Key_Actions::MOVE_BACK;
    //    keyActions["MOVE_FORWARD"] = (int)EngineFramework::Key_Actions::MOVE_FORWARD;
    //    keyActions["MOVE_LEFT"] = (int)EngineFramework::Key_Actions::MOVE_LEFT;
    //    keyActions["MOVE_RIGHT"] = (int)EngineFramework::Key_Actions::MOVE_RIGHT;
    //    keyActions["OPEN_EDITOR"] = (int)EngineFramework::Key_Actions::OPEN_EDITOR;

    //}

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

        s_Lua.set_function("Physics_ApplyImpulse",
            [](uint32_t entityId, float x, float y, float z)
            {
                entt::entity e = static_cast<entt::entity>(entityId);
                PhysicsWorld::Get().ApplyImpulse(e, x, y, z);
            });

        s_Lua.set_function("Physics_ExplodeGroup",
            [](sol::table parts, float strength)
            {
                std::vector<entt::entity> entities;
                for (auto pair : parts)
                {
                    if (pair.second.is<uint32_t>())
                    {
                        entities.push_back(static_cast<entt::entity>(pair.second.as<uint32_t>()));
                    }
                }
                PhysicsWorld::Get().ExplodeGroup(entities, strength);
            });

        s_Lua.set_function("Physics_RadialExplosion",
            [](sol::table parts, float x, float y, float z, float strength)
            {
                std::vector<entt::entity> entities;
                for (auto pair : parts)
                {
                    if (pair.second.is<uint32_t>())
                    {
                        entities.push_back(static_cast<entt::entity>(pair.second.as<uint32_t>()));
                    }
                }
                PhysicsWorld::Get().ApplyRadialExplosion(entities, x, y, z, strength);
            });

        // ===============================================================
        // Light Component bindings
        // ===============================================================
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
        sol::table audioEnums = s_Lua.create_named_table("Audio");

        audioEnums["Category_SFX"] = (int)AudioComponent::AudioCategory::SFX;
        audioEnums["Category_BGM"] = (int)AudioComponent::AudioCategory::BGM;
        audioEnums["Category_UI"] = (int)AudioComponent::AudioCategory::UI;
        audioEnums["Category_Ambience"] = (int)AudioComponent::AudioCategory::Ambience;

        audioEnums["Spatial_2D"] = (int)AudioComponent::SpatialMode::TwoD;
        audioEnums["Spatial_3D"] = (int)AudioComponent::SpatialMode::ThreeD;

        // Bind Slot (formerly AudioClip)
        s_Lua.new_usertype<AudioComponent::Slot>("AudioSlot",
            // ID
            "GetID", [](AudioComponent::Slot& s) { return s.id; },
            "SetID", [](AudioComponent::Slot& s, const std::string& v) { s.id = v; },

            // Volume / Pitch / Loop
            "GetVolume", [](AudioComponent::Slot& s) { return s.volume; },
            "SetVolume", [](AudioComponent::Slot& s, float v) { s.volume = v; },

            "GetPitch", [](AudioComponent::Slot& s) { return s.pitch; },
            "SetPitch", [](AudioComponent::Slot& s, float v) { s.pitch = v; },

            "GetLoop", [](AudioComponent::Slot& s) { return s.loop; },
            "SetLoop", [](AudioComponent::Slot& s, bool v) { s.loop = v; },

            // PlayOnStart
            "GetPlayOnStart", [](AudioComponent::Slot& s) { return s.playOnStart; },
            "SetPlayOnStart", [](AudioComponent::Slot& s, bool v) { s.playOnStart = v; },

            // Category / Spatial (as ints in Lua)
            "GetCategory", [](AudioComponent::Slot& s) { return (int)s.category; },
            "SetCategory", [](AudioComponent::Slot& s, int v) { s.category = (AudioComponent::AudioCategory)v; },

            "GetSpatial", [](AudioComponent::Slot& s) { return (int)s.spatial; },
            "SetSpatial", [](AudioComponent::Slot& s, int v) { s.spatial = (AudioComponent::SpatialMode)v; },

            // 3D distances
            "GetMinDistance", [](AudioComponent::Slot& s) { return s.minDistance; },
            "SetMinDistance", [](AudioComponent::Slot& s, float d) { s.minDistance = d; },

            "GetMaxDistance", [](AudioComponent::Slot& s) { return s.maxDistance; },
            "SetMaxDistance", [](AudioComponent::Slot& s, float d) { s.maxDistance = d; }
        );

        // Bind AudioComponent itself
        s_Lua.new_usertype<AudioComponent>("AudioComponent",
            "GetSlotCount", [](AudioComponent& a) { return (int)a.Slots.size(); },

            "GetSlot", [](AudioComponent& a, std::size_t index) -> AudioComponent::Slot* {
                if (index >= a.Slots.size()) return nullptr;
                return &a.Slots[index];
            },

            "FindSlot", [](AudioComponent& a, const std::string& id) -> AudioComponent::Slot* {
                return a.FindSlot(id);
            },

            "AddSlot", [](AudioComponent& a, const std::string& id) -> AudioComponent::Slot* {
                return &a.AddSlot(id);
            },

            "RemoveSlot", [](AudioComponent& a, std::size_t index) {
                a.RemoveSlot(index);
            }
        );

        // ===============================================================
        // AudioSystem bindings (play/stop by entity + slot ID)
        // ===============================================================

        // Use global functions (recommended) so scripts can trigger audio reliably
        s_Lua.set_function("Audio_PlaySlot",
            [](uint32_t entityId, const std::string& slotId)
            {
                entt::entity e = static_cast<entt::entity>(entityId);
                EngineFramework::AudioSystem::GetInstance().PlayEntitySlot(e, slotId);
            });

        s_Lua.set_function("Audio_StopSlot",
            [](uint32_t entityId, const std::string& slotId)
            {
                entt::entity e = static_cast<entt::entity>(entityId);
                EngineFramework::AudioSystem::GetInstance().StopEntitySlot(e, slotId);
            });

        s_Lua.set_function("Audio_StopAllForEntity",
            [](uint32_t entityId)
            {
                entt::entity e = static_cast<entt::entity>(entityId);
                EngineFramework::AudioSystem::GetInstance().StopAllForEntity(e);
            });

        // Optional: mixer controls from Lua (useful for settings menus)
        s_Lua.set_function("Audio_SetGroupVolume",
            [](int group, float vol)
            {
                EngineFramework::AudioSystem::GetInstance().SetGroupVolume(
                    (EngineFramework::AudioGroup)group, vol);
            });

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

        ScriptSystem::Init_Keyaction();

        s_Lua.new_usertype<EngineFramework::InputManager>("InputManager",
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
            ),
            "GetWindowSize", [](EngineFramework::InputManager& im) {
                auto [w, h] = im.GetWindowSize();
                return std::make_tuple(w, h);
            }
            
            /*"GetAction", sol::overload(
                static_cast<bool(*)(int)>(&EngineFramework::InputManager::GetAction),
                static_cast<bool(*)(const std::string&)>(&EngineFramework::InputManager::GetAction)
            )*/

        );

        sol::table inputTable = s_Lua.create_named_table("Input");

        

        // ===============================================================
        // Query helpers
        // ===============================================================
        sol::table queryTable = s_Lua.create_named_table("Query");

        queryTable.set_function("IsEntityInRange",
            [](uint32_t entityAId, uint32_t entityBId, float range) -> bool
            {
                if (!s_RuntimeRegistry) return false;
                if (range < 0.0f) return false;

                entt::entity a = static_cast<entt::entity>(entityAId);
                entt::entity b = static_cast<entt::entity>(entityBId);

                if (!s_RuntimeRegistry->valid(a) || !s_RuntimeRegistry->valid(b))
                    return false;

                if (!s_RuntimeRegistry->all_of<TransformComponent>(a) ||
                    !s_RuntimeRegistry->all_of<TransformComponent>(b))
                    return false;

                const auto& ta = s_RuntimeRegistry->get<TransformComponent>(a);
                const auto& tb = s_RuntimeRegistry->get<TransformComponent>(b);

                const glm::vec3 d = tb.position - ta.position;
                const float distSq = glm::dot(d, d);
                const float rSq = range * range;

                return distSq <= rSq;
            });


#if /*defined(DEBUG) && */defined(PLATFORM_WINDOWS)
        /*inputTable.set_function("GetAction", [](const std::string& action) {
            return EngineFramework::InputManager::GetAction(action);
            });
        inputTable.set_function("GetActionDown", [](const std::string& action) {
            return EngineFramework::InputManager::GetActionDown(action);
            });
        inputTable.set_function("GetActionUp", [](const std::string& action) {
            return EngineFramework::InputManager::GetActionUp(action);
            });*/

        inputTable.set_function("GetAction", sol::overload(
            [](const std::string& action) {
                return EngineFramework::InputManager::GetAction(action);
            },
            [](int action) {
                return EngineFramework::InputManager::GetAction(action);
            }
        ));

        inputTable.set_function("GetActionDown", sol::overload(
            [](const std::string& action) {
                return EngineFramework::InputManager::GetActionDown(action);
            },
            [](int action) {
                return EngineFramework::InputManager::GetActionDown(action);
            }
        ));

        inputTable.set_function("GetActionUp", sol::overload(
            [](const std::string& action) {
                return EngineFramework::InputManager::GetActionUp(action);
            },
            [](int action) {
                return EngineFramework::InputManager::GetActionUp(action);
            }
        ));

        inputTable.set_function("SetCursorLocked", sol::overload(
            [](bool locked) {
                EngineFramework::InputManager::SetCursorLocked(locked);
            },
            [](int locked) {
                EngineFramework::InputManager::SetCursorLocked(locked != 0);
            }
        ));

        inputTable.set_function("SetCursorVisible", sol::overload(
            [](bool visible) {
                EngineFramework::InputManager::SetCursorVisible(visible);
            },
            [](int visible) {
                EngineFramework::InputManager::SetCursorVisible(visible != 0);
            }
        ));

#else
        // optional: compile-safe stubs for non-editor builds
        inputTable.set_function("GetAction", [](const std::string&) { return false; });
        inputTable.set_function("GetActionDown", [](const std::string&) { return false; });
        inputTable.set_function("GetActionUp", [](const std::string&) { return false; });
        inputTable.set_function("SetCursorLocked", [](bool) {});
        inputTable.set_function("SetCursorVisible", [](bool) {});
#endif

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
            glm::vec3 rate = EngineFramework::InputManager::GetGyroscopeRotationRate();
            return std::make_tuple(rate.x, rate.y, rate.z);
        });
        inputTable.set_function("GetWindowSize", []() {
            auto [w, h] = EngineFramework::InputManager::GetInstance().GetWindowSize();
            return std::make_tuple(w, h);
         });

        LOGI("[ScriptSystem] InputManager bindings complete (static table approach)");




        



        // ===============================================================
        // CameraComponent bindings
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

        sol::table cameraTable = s_Lua.create_named_table("Camera");
        cameraTable.set_function("onCursor", &Camera::onCursor);
        cameraTable.set_function("onScroll", &Camera::onScroll);
        cameraTable.set_function("setType", &Camera::setType);
        cameraTable.set_function("Move", &Camera::Move_Cam_and_Target);
        cameraTable.set_function("getLookAt", &Camera::getLookAt);
        cameraTable.set_function("getPerspective", &Camera::getPerspective);

        // NOTE: these still reference `registry` as in your original code.
        // Ideally swap to `reg` (passed in) later.
        s_Lua.set_function("GetMainCamera", []() -> CameraComponent* {
            auto view = registry.view<CameraComponent>();
            if (!view.empty()) {
                return &view.get<CameraComponent>(view.front());
            }
            return nullptr;
            });

        s_Lua.set_function("GetEntityTransform", [](entt::entity e) -> TransformComponent* {
            if (registry.all_of<TransformComponent>(e)) {
                return &registry.get<TransformComponent>(e);
            }
            return nullptr;
            });

        s_Lua.set_function("GetEntityUI", [](entt::entity e) -> UIComponent* {
            if (registry.all_of<UIComponent>(e)) {
                return &registry.get<UIComponent>(e);
            }
            return nullptr;
            });

        s_Lua.set_function("GetEntityAudio", [](entt::entity e) -> AudioComponent* {
            if (registry.all_of<AudioComponent>(e))
                return &registry.get<AudioComponent>(e);
            return nullptr;
            });

        // ===============================================================
        // GLFW Key Constants
        // ===============================================================
        s_Lua["GLFW_KEY_W"] = 87;
        s_Lua["GLFW_KEY_A"] = 65;
        s_Lua["GLFW_KEY_S"] = 83;
        s_Lua["GLFW_KEY_D"] = 68;
        s_Lua["GLFW_KEY_F"] = 70;
        s_Lua["GLFW_KEY_Q"] = 81;
        s_Lua["GLFW_KEY_E"] = 69;
        s_Lua["GLFW_KEY_C"] = 67;
        s_Lua["GLFW_KEY_SPACE"] = 32;
        s_Lua["GLFW_KEY_SHIFT"] = 340;

        s_Lua["GLFW_MOUSE_BUTTON_LEFT"] = 0;
        s_Lua["GLFW_MOUSE_BUTTON_RIGHT"] = 1;
        s_Lua["GLFW_MOUSE_BUTTON_MIDDLE"] = 2;

        LOGI("[ScriptSystem] Lua bindings initialized successfully.");
#endif
    }

    void ScriptSystem::Shutdown() {}

    // ------------------------------------------------------------------
    // NEW: Safe reload request API (call from editor/ImGui)
    // ------------------------------------------------------------------
    void ScriptSystem::RequestReloadAll() {
#if defined(ENGINE_ENABLE_LUA)
        std::scoped_lock lock(s_ReloadMutex);
        s_ReloadAllRequested = true;
#endif
    }

    void ScriptSystem::RequestReloadEntity(entt::entity e) {
#if defined(ENGINE_ENABLE_LUA)
        std::scoped_lock lock(s_ReloadMutex);
        s_ReloadEntityRequests.insert(e);
#else
        (void)e;
#endif
    }

    // ------------------------------------------------------------------
    // Runtime start/stop
    // ------------------------------------------------------------------
    void ScriptSystem::OnRuntimeStart(entt::registry& reg) {
#if defined(ENGINE_ENABLE_LUA)
        LOGI("[ScriptSystem] OnRuntimeStart called - initializing Lua scripts...");

        s_RuntimeRegistry = &reg;

        reg.view<LuaScriptComponent>().each([](auto& comp) {
            for (auto& inst : comp.Scripts)
                inst.Loaded = false;
            });

        auto cameraView = reg.view<CameraComponent, LuaScriptComponent>();
        for (auto [entity, camera, scripts] : cameraView.each()) {
            for (auto& inst : scripts.Scripts) {
                LoadLuaFor(reg, entity, inst);
            }
        }

        auto otherView = reg.view<LuaScriptComponent>(entt::exclude<CameraComponent>);
        for (auto [entity, scripts] : otherView.each()) {
            for (auto& inst : scripts.Scripts) {
                LoadLuaFor(reg, entity, inst);
            }
        }
#endif
    }

    void ScriptSystem::OnRuntimeStop(entt::registry& reg) {
        ;
        auto nativeView = reg.view<NativeScriptComponent>();
        for (auto [e, nsc] : nativeView.each()) {
            if (nsc.Instance) { nsc.Instance->OnDestroy(); DestroyNativeInstance(nsc); }
        }

#if defined(ENGINE_ENABLE_LUA)
        s_RuntimeRegistry = nullptr;

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

            // -----------------------------------------------------------
            // SAFE POINT: apply queued reload requests BEFORE iterating
            // -----------------------------------------------------------
            bool doReloadAll = false;
            std::unordered_set<entt::entity> entitiesToReload;

            {
                std::scoped_lock lock(s_ReloadMutex);
                doReloadAll = s_ReloadAllRequested;
                s_ReloadAllRequested = false;
                entitiesToReload.swap(s_ReloadEntityRequests);
            }

            if (doReloadAll) {
                auto v = reg.view<LuaScriptComponent>();
                for (auto e2 : v) {
                    auto& comp2 = reg.get<LuaScriptComponent>(e2);
                    for (auto& inst2 : comp2.Scripts) {
                        LoadLuaFor(reg, e2, inst2);
                    }
                }
                LOGI("[ScriptSystem] Reloaded ALL Lua scripts (requested).");
            }
            else if (!entitiesToReload.empty()) {
                for (auto e2 : entitiesToReload) {
                    if (!reg.valid(e2)) continue;
                    if (!reg.any_of<LuaScriptComponent>(e2)) continue;

                    auto& comp2 = reg.get<LuaScriptComponent>(e2);
                    for (auto& inst2 : comp2.Scripts) {
                        LoadLuaFor(reg, e2, inst2);
                    }
                }
                LOGI("[ScriptSystem] Reloaded %zu entities' Lua scripts (requested).", entitiesToReload.size());
            }

            // -----------------------------------------------------------
            // Normal Lua update
            // -----------------------------------------------------------
            auto view = reg.view<LuaScriptComponent>();
            for (auto [e, comp] : view.each()) {
                if (!comp.Enabled) continue;

                for (auto& inst : comp.Scripts) {
                    if (!inst.Loaded)
                        LoadLuaFor(reg, e, inst);

                    if (inst.Loaded && inst.FnOnUpdate.valid()) {
                        try {
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

    void ScriptSystem::LoadLuaFor(entt::registry& reg,
        entt::entity e,
        LuaScriptComponent::LuaInstance& inst)
    {
        // IMPORTANT CHANGE:
        // Hot reload should call on_destroy to allow scripts to clean up.
        UnloadLuaFor(inst, /*callOnDestroy*/true);

        if (inst.ScriptGuid.m_Value == 0) {
            LOGE("[LuaRuntime] Invalid Script GUID; skipping load (entity %u)",
                static_cast<unsigned>(e));
            inst.Loaded = false;
            return;
        }

        auto full_guid = xresource::full_guid{
            inst.ScriptGuid,
            EngineFramework::AssetManagerSystem::SCRIPT_GUID };

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

            // Prepare entity-specific data
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

            // NOTE: these still use `registry` like your original code.
            LightComponent* entityLight = nullptr;
            if (registry.all_of<LightComponent>(e)) {
                entityLight = &registry.get<LightComponent>(e);
            }

            UIComponent* entityUI = nullptr;
            if (registry.all_of<UIComponent>(e)) {
                entityUI = &registry.get<UIComponent>(e);
            }

            PhysicsComponent* entityPhysics = nullptr;
            if (registry.all_of<PhysicsComponent>(e)) {
                entityPhysics = &registry.get<PhysicsComponent>(e);
            }

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
    // Hot reload (immediate)
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
