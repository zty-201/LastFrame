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
#include "global.hpp"
#include "CoreEngine.hpp"

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
#include "CharacterControllerComponent.h"
#include "PhysicsWorld.hpp"

// For Audio
#include "AudioComponent.h"
#include "Audio.hpp"

// For Particles
#include "ParticleEmitterComponent.h"

// For Game/Scene
#include "System_GameObjectSystem.hpp"
#include "SceneManager.hpp"

//For platform android vs windows
#include "Platform.h"

#include "StateMachineSystem.hpp"
#include "UISystem.hpp"

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

    void ScriptSystem::OnButtonEvent(const Button_Event& e) {
        if (!s_RuntimeRegistry) return;

        //LOGI("[ScriptSystem] Button event received: %s", e.Event_Name.c_str());

        // Dispatch to all entities with Lua scripts
        auto view = s_RuntimeRegistry->view<LuaScriptComponent>();
        for (auto [entity, comp] : view.each()) {
            if (!comp.Enabled) continue;

            for (auto& inst : comp.Scripts) {
                if (!inst.Loaded) continue;

                if (inst.Env.lua_state() == nullptr) {
                    continue;
                }

                // Build handler function name from event_trigger
                // e.g., if event_trigger = "PlayButton", looks for on_button_PlayButton()
                std::string handlerName = "on_button_" + e.Event_Name;

                sol::optional<sol::protected_function> funcOption =
                    inst.Env.get<sol::optional<sol::protected_function>>(handlerName);

                if (funcOption && funcOption.value().valid()) {
                    auto func = funcOption.value();
                    try {
                        auto result = func(inst.Self);
                        if (!result.valid()) {
                            sol::error err = result;
                            LOGE("[ButtonEvent Error] %s: %s", handlerName.c_str(), err.what());
                        }
                    }
                    catch (const sol::error& err) {
                        LOGE("[ButtonEvent Exception] %s: %s", handlerName.c_str(), err.what());
                    }
                }
            }
        }
    }

    void DispatchLuaEvent(entt::registry& reg, entt::entity entity, const std::string& funcName, entt::entity param)
    {
        if (!reg.valid(entity)) return;

        auto* luaComp = reg.try_get<LuaScriptComponent>(entity);
        if (!luaComp || !luaComp->Enabled) return;

        for (auto& inst : luaComp->Scripts)
        {
            if (!inst.Loaded) continue;

            if (inst.Env.lua_state() == nullptr) {
                //LOGE("[LuaEvent] Script environment has NULL state! Skipping %s", funcName.c_str());
                continue;
            }

            sol::optional<sol::protected_function> funcOption = inst.Env.get<sol::optional<sol::protected_function>>(funcName);
            if (funcOption && funcOption.value().valid())
            {
                auto func = funcOption.value();
                try {
                    auto result = func(inst.Self, static_cast<uint32_t>(param));
                    if (!result.valid())
                    {
                        sol::error err = result;
                        LOGE("[LuaEvent Error] %s: %s", funcName.c_str(), err.what());
                    }
                }
                catch (const sol::error& err)
                {
                    LOGE("[LuaEvent Exception] %s: %s", funcName.c_str(), err.what());
                }
            }
        }
    }

    void ScriptSystem::OnTriggerEnter(const Event_OnTriggerEnter& e) {
        if (!s_RuntimeRegistry) return;

        //Something entered
        DispatchLuaEvent(*s_RuntimeRegistry, e.trigger, "on_trigger_enter", e.other);
        //Notify the Player "I entered a zone"
        DispatchLuaEvent(*s_RuntimeRegistry, e.other, "on_trigger_enter", e.trigger);
    }

    void ScriptSystem::OnTriggerExit(const Event_OnTriggerExit& e) {
        if (!s_RuntimeRegistry) return;

        // Notify both
        DispatchLuaEvent(*s_RuntimeRegistry, e.trigger, "on_trigger_exit", e.other);
        DispatchLuaEvent(*s_RuntimeRegistry, e.other, "on_trigger_exit", e.trigger);
    }

    void ScriptSystem::OnTriggerStay(const Event_OnTriggerStay& e) {
        if (!s_RuntimeRegistry) return;

        // Notify both
        DispatchLuaEvent(*s_RuntimeRegistry, e.trigger, "on_trigger_stay", e.other);
        DispatchLuaEvent(*s_RuntimeRegistry, e.other, "on_trigger_stay", e.trigger);
    }

    void ScriptSystem::Init() {
#if defined(ENGINE_ENABLE_LUA)
        s_Lua.open_libraries(sol::lib::base, sol::lib::package, sol::lib::math,
            sol::lib::table, sol::lib::string, sol::lib::io);

        // ===============================================================
        // Create SHARED table for cross-script communication
        // ===============================================================
        s_Lua["SHARED"] = s_Lua.create_table();
        s_Lua["SHARED"]["UI"] = s_Lua.create_table();
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
        // UI bindings (Lua -> UI_System)
        // ===============================================================

        s_Lua.set_function("GetEntityName",
            [](uint32_t entityId) -> std::string
            {
                entt::entity e = static_cast<entt::entity>(entityId);
                return EngineFramework::GameObjectSystem::GetInstance().GetEntityName(e);
            });

        s_Lua.set_function("StateMachine_SetTrigger", [](const std::string& triggerName)
            {
                auto& sm = EngineFramework::StateMachineSystem::GetInstance().GetMachine();
                sm.SetTrigger(triggerName);
            });

        s_Lua.set_function("UI_StartPseudoCutscene", []()
            {
                // If your UI_System is NOT singleton and is purely static:
                // EngineFramework::UI_System::StartPseudoCutscene();

                // If UI_System is a singleton:
                EngineFramework::UI_System::GetInstance().StartPseudoCutscene();
            });

        s_Lua.set_function("UI_StopPseudoCutscene", []()
            {
                // stop + close all menus so you instantly leave the cutscene UI
                EngineFramework::UI_System::GetInstance().StopCutsceneSequence(true);
            });

        s_Lua.set_function("UI_IsCutscenePlaying", []() -> bool
            {
                return EngineFramework::UI_System::GetInstance().IsCutscenePlaying();
            });

        s_Lua.set_function("UI_DisplayMenu", [](const std::string& menuName)
            {
                // EngineFramework::UI_System::displayMenu(menuName);  // static-style
                EngineFramework::UI_System::GetInstance().displayMenu(menuName);
            });
        s_Lua.set_function("UI_CloseMenu", [](const std::string& menuName)
            {
                EngineFramework::UI_System::GetInstance().closeMenu(menuName);
            });
        s_Lua.set_function("UI_IsMenuActive", [](const std::string& menuName) -> bool
            {
                return EngineFramework::UI_System::GetInstance().IsMenuActive(menuName);
            });

        s_Lua.set_function("UI_CloseAllMenus", [](sol::optional<bool> saveState)
            {
                bool value = saveState.value_or(false);
                EngineFramework::UI_System::GetInstance().closeAllMenu(value);
            });

        s_Lua.set_function("UI_CloseMenu", [](const std::string& menuName)
            {
                // EngineFramework::UI_System::displayMenu(menuName);  // static-style
                EngineFramework::UI_System::GetInstance().closeMenu(menuName);
            });
        s_Lua.set_function("UI_DisplayMenu_InvisibleLayer", [](const std::string& menuName)
            {
                // EngineFramework::UI_System::displayMenu(menuName);  // static-style
                EngineFramework::UI_System::GetInstance().displayMenu_InvisibleLayer(menuName);
            });
        //

        s_Lua.set_function("UI_is_MenuOpen", [](const std::string& menuName) -> bool
            {
                return EngineFramework::UI_System::GetInstance().is_MenuOpen(menuName);
            });

        

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

        s_Lua.set_function("Physics_CheckCollision",
            [](uint32_t idA, uint32_t idB) -> bool
            {
                entt::entity eA = static_cast<entt::entity>(idA);
                entt::entity eB = static_cast<entt::entity>(idB);
                return PhysicsWorld::Get().AreEntitiesColliding(eA, eB);
            });

        s_Lua.set_function("Character_SetVelocity", [&](uint32_t entityID, float x, float y, float z)
            {
                entt::entity e = (entt::entity)entityID;
                if (registry.valid(e) && registry.any_of<CharacterControllerComponent>(e)) {
                    auto& charCtrl = registry.get<CharacterControllerComponent>(e);
                    charCtrl.controlVelocity = glm::vec3(x, y, z);
                }
            });

        s_Lua.set_function("Character_IsGrounded", [&](uint32_t entityID) -> bool 
            {
                entt::entity e = (entt::entity)entityID;
                if (registry.valid(e) && registry.any_of<CharacterControllerComponent>(e))
                {
                    return registry.get<CharacterControllerComponent>(e).isGrounded;
                }
                return false;
            });

        s_Lua.set_function("Character_GetVelocity", [&](uint32_t entityID) -> std::tuple<float, float, float> {
            entt::entity e = (entt::entity)entityID;
            if (registry.valid(e) && registry.any_of<CharacterControllerComponent>(e)) {
                auto vel = registry.get<CharacterControllerComponent>(e).controlVelocity;
                return std::make_tuple(vel.x, vel.y, vel.z);
            }
            return std::make_tuple(0.f, 0.f, 0.f);
            });

        s_Lua.set_function("Character_SetGravityFactor", [&](uint32_t entityID, float factor) {
            entt::entity e = (entt::entity)entityID;
            if (registry.valid(e) && registry.any_of<CharacterControllerComponent>(e))
            {
                registry.get<CharacterControllerComponent>(e).gravityFactor = factor;
            }
            });

        s_Lua.set_function("Transform_GetRotation", [&](uint32_t entityID) -> std::tuple<float, float, float> {
            entt::entity e = (entt::entity)entityID;
            if (registry.valid(e) && registry.any_of<TransformComponent>(e))
            {
                const auto& transform = registry.get<TransformComponent>(e);
                return std::make_tuple(transform.rotation.x, transform.rotation.y, transform.rotation.z);
            }
            return std::make_tuple(0.f, 0.f, 0.f);
            });

        s_Lua.set_function("Transform_SetRotation", [&](uint32_t entityID, float x, float y, float z) {
            entt::entity e = (entt::entity)entityID;
            if (registry.valid(e) && registry.any_of<TransformComponent>(e))
            {
                auto& transform = registry.get<TransformComponent>(e);
                transform.rotation = glm::vec3(x, y, z);
                glm::quat q = glm::quat(glm::radians(transform.rotation));

                if (PhysicsWorld::Get().HasBody(e))
                {
                    PhysicsWorld::Get().SetRotation(e, q);
                }
                
                if (PhysicsWorld::Get().HasCharacter(e))
                {
                    PhysicsWorld::Get().SetCharacterRotation(e, q);
                }
            }
            });

        // ===============================================================
        // Light Component bindings
        // ===============================================================
        s_Lua.new_usertype<LightComponent>("LightComponent",
            "GetIntensity", [](LightComponent& light) {
                return light.intensity;
            },
            "SetIntensity", [](LightComponent& light, float i) {
                light.intensity = i;
            },
            "SetColor", [](LightComponent& light, float r, float g, float b) {
                light.color = glm::vec3(r, g, b);
			},
            "GetColor", [](LightComponent& light) {
                return std::make_tuple(light.color.r, light.color.g, light.color.b);
            }
        );

        LOGI("[ScriptSystem] LightComponent bindings complete.");

        // ===============================================================
        // ParticleEmitterComponent bindings
        // ===============================================================
        s_Lua.new_usertype<ParticleEmitterComponent>("ParticleEmitterComponent",
            "isEmitting", sol::property(
                [](ParticleEmitterComponent& p) { return p.isEmitting; },
                [](ParticleEmitterComponent& p, bool v) { p.isEmitting = v; }
            )
        );


        // ===============================================================
        // Game / Scene bindings
        // ===============================================================
        

        s_Lua.set_function("Scene_RequestRestart", []()
            {
                EngineFramework::CoreEngine::RequestSceneRestart();
            });

        s_Lua.set_function("Scene_QuitGame", []()
            {
                EngineFramework::CoreEngine::RequestQuit();
            });

        s_Lua.set_function("LoadScene", [&](const std::string& sceneName) {
            std::string fileName = sceneName;
            if (fileName.find(".json") == std::string::npos)
            {
                fileName += ".json";
            }

            std::string path = "Extern/Scenes/" + fileName;

            std::string sceneString = EngineFramework::AssetManagerSystem::GetStringFromFile(path);

            if (!sceneString.empty())
            {
                LOGI("[Lua] Queueing scene load: %s", fileName.c_str());
                EngineFramework::GameObjectSystem::GetInstance().QueueLoadScene(sceneString);
            }
            else
            {
                LOGE("[Lua ERROR] LoadScene failed: Could not find or read %s", path.c_str());
            }
            });

        // ===============================================================
        // AudioComponent bindings
        // ===============================================================
        // Used by scripts to configure AudioSlot behaviour.
        //
        // Categories control mixer routing and global volume sliders:
        //   - SFX       : gameplay sounds (footsteps, doors, impacts)
        //   - BGM       : background music
        //   - UI        : menu / interface sounds
        //   - Ambience  : environmental sounds (wind, rain, insects)
        //
        // Spatial modes:
        //   - 2D : non-positional (UI, music)
        //   - 3D : positional, follows entity transform
        // ===============================================================
        sol::table audioEnums = s_Lua.create_named_table("Audio");

        audioEnums["Category_SFX"] = (int)AudioComponent::AudioCategory::SFX;
        audioEnums["Category_BGM"] = (int)AudioComponent::AudioCategory::BGM;
        audioEnums["Category_UI"] = (int)AudioComponent::AudioCategory::UI;
        audioEnums["Category_Ambience"] = (int)AudioComponent::AudioCategory::Ambience;

        audioEnums["Spatial_2D"] = (int)AudioComponent::SpatialMode::TwoD;
        audioEnums["Spatial_3D"] = (int)AudioComponent::SpatialMode::ThreeD;

        // ===============================================================
        // Audio bindings (Master volume control)
        // ===============================================================

        // Expose AudioGroup enum to Lua (optional but nice)
        {
            sol::table audioGroup = s_Lua.create_named_table("AudioGroup");
            audioGroup["Master"] = (int)EngineFramework::AudioGroup::Master;
            audioGroup["BGM"] = (int)EngineFramework::AudioGroup::BGM;
            audioGroup["SFX"] = (int)EngineFramework::AudioGroup::SFX;
            audioGroup["UI"] = (int)EngineFramework::AudioGroup::UI;
            audioGroup["Ambience"] = (int)EngineFramework::AudioGroup::Ambience;
        }

        // Master volume setter (recommended for your use case)
        s_Lua.set_function("Audio_SetMasterVolume",
            [](float vol)
            {
                EngineFramework::AudioSystem::GetInstance().SetMasterVolume(vol);
            });

        // (Optional) If you want a getter later, you currently don't have one in AudioSystem.
        // You can store in SHARED in Lua (as we did) without needing a getter.

        // ===============================================================
        // AudioSlot (Lua)
        // ===============================================================
        // A Slot represents ONE sound configuration inside an AudioComponent.
        // Each slot is identified by a string ID (e.g. "Footstep", "Wind").
        //
        // Slots are authored in the editor and triggered from Lua.
        // ===============================================================
        s_Lua.new_usertype<AudioComponent::Slot>("AudioSlot",
            // Slot ID
            // Used by Lua to play/stop this sound (string-based lookup).
            "GetID", [](AudioComponent::Slot& s) { return s.id; },
            "SetID", [](AudioComponent::Slot& s, const std::string& v) { s.id = v; },

            // Volume / Pitch / Loop
            "GetVolume", [](AudioComponent::Slot& s) { return s.volume; },
            "SetVolume", [](AudioComponent::Slot& s, float v) { s.volume = v; },

            "GetPitch", [](AudioComponent::Slot& s) { return s.pitch; },
            "SetPitch", [](AudioComponent::Slot& s, float v) { s.pitch = v; },

            "GetLoop", [](AudioComponent::Slot& s) { return s.loop; },
            "SetLoop", [](AudioComponent::Slot& s, bool v) { s.loop = v; },

            // Play automatically when simulation starts
            "GetPlayOnStart", [](AudioComponent::Slot& s) { return s.playOnStart; },
            "SetPlayOnStart", [](AudioComponent::Slot& s, bool v) { s.playOnStart = v; },

            // Routing & spatial behaviour
            // Category affects mixer group (SFX / BGM / UI / Ambience)
            "GetCategory", [](AudioComponent::Slot& s) { return (int)s.category; },
            "SetCategory", [](AudioComponent::Slot& s, int v) { s.category = (AudioComponent::AudioCategory)v; },

            "GetSpatial", [](AudioComponent::Slot& s) { return (int)s.spatial; },
            "SetSpatial", [](AudioComponent::Slot& s, int v) { s.spatial = (AudioComponent::SpatialMode)v; },

            // 3D attenuation distances (only used for Spatial_3D)
            // minDistance:
            //   Distance (in world units) where the sound plays at FULL volume.
            //   Within this range, volume does NOT fall off.
            //
            // maxDistance:
            //   Distance where the sound becomes very quiet / near silent.
            //   Volume fades smoothly between minDistance and maxDistance.
            //
            // Notes:
            // - Larger minDistance = sound stays loud over a larger area
            // - Smaller minDistance = sound drops off quickly
            // - Ambience usually needs LARGE minDistance
            "GetMinDistance", [](AudioComponent::Slot& s) { return s.minDistance; },
            "SetMinDistance", [](AudioComponent::Slot& s, float d) { s.minDistance = d; },

            "GetMaxDistance", [](AudioComponent::Slot& s) { return s.maxDistance; },
            "SetMaxDistance", [](AudioComponent::Slot& s, float d) { s.maxDistance = d; }
        );

        // ===============================================================
        // AudioComponent (Lua)
        // ===============================================================
        // Component that owns multiple AudioSlots.
        // Usually configured in the editor, not created at runtime.
        // ===============================================================
        s_Lua.new_usertype<AudioComponent>("AudioComponent",

            // Number of slots on this component
            "GetSlotCount", [](AudioComponent& a) { return (int)a.Slots.size(); },

            // Access slot by index (returns nil if out of range)
            "GetSlot", [](AudioComponent& a, std::size_t index) -> AudioComponent::Slot* {
                if (index >= a.Slots.size()) return nullptr;
                return &a.Slots[index];
            },

            // Find slot by string ID
            "FindSlot", [](AudioComponent& a, const std::string& id) -> AudioComponent::Slot* {
                return a.FindSlot(id);
            },

            // Add or return a slot with the given ID
            "AddSlot", [](AudioComponent& a, const std::string& id) -> AudioComponent::Slot* {
                return &a.AddSlot(id);
            },

            // Remove slot by index
            "RemoveSlot", [](AudioComponent& a, std::size_t index) {
                a.RemoveSlot(index);
            }
        );

        // ===============================================================
        // AudioSystem  Playback helpers (Lua)
        // ===============================================================
        // Recommended way to trigger audio from scripts.
        // Uses entity + slot ID instead of raw sound assets.
        // ===============================================================

        // Play a slot on an entity's AudioComponent
        // Example: Audio_PlaySlot(EntityID, "Footstep")
        s_Lua.set_function("Audio_PlaySlot",
            [](uint32_t entityId, const std::string& slotId)
            {
                entt::entity e = static_cast<entt::entity>(entityId);
                EngineFramework::AudioSystem::GetInstance().PlayEntitySlot(e, slotId);
            });

        // Stop a specific slot on an entity
        s_Lua.set_function("Audio_StopSlot",
            [](uint32_t entityId, const std::string& slotId)
            {
                entt::entity e = static_cast<entt::entity>(entityId);
                EngineFramework::AudioSystem::GetInstance().StopEntitySlot(e, slotId);
            });

        // Stop all playing slots for an entity
        s_Lua.set_function("Audio_StopAllForEntity",
            [](uint32_t entityId)
            {
                entt::entity e = static_cast<entt::entity>(entityId);
                EngineFramework::AudioSystem::GetInstance().StopAllForEntity(e);
            });

        // Set the volume of a specific playing slot on an entity (0.0 - 5.0)
        // Example: Audio_SetSlotVolume(EntityID, "GirlLaughing", 0.8)
        s_Lua.set_function("Audio_SetSlotVolume",
            [](uint32_t entityId, const std::string& slotId, float volume)
            {
                entt::entity e = static_cast<entt::entity>(entityId);
                EngineFramework::AudioSystem::GetInstance().SetEntitySlotVolume(e, slotId, volume);
            });

        // ===============================================================
        // AudioSystem  Mixer controls (Lua)
        // ===============================================================
        // Used for settings menus (volume sliders).
        // ===============================================================
        s_Lua.set_function("Audio_SetGroupVolume",
            [](int group, float vol)
            {
                EngineFramework::AudioSystem::GetInstance().SetGroupVolume(
                    (EngineFramework::AudioGroup)group, vol);
            });

        // ===============================================================
        // AudioSystem  Environment controls (Lua)
        // ===============================================================
        // Controls ambience muffling based on environment.
        //
        // 0.0 = outdoor (clear ambience)
        // 1.0 = indoor (muffled ambience)
        //
        // Typically driven by trigger zones.
        // ===============================================================
        s_Lua.set_function("Audio_SetIndoorFactor",
            [](float factor)
            {
                EngineFramework::AudioSystem::GetInstance().SetIndoorFactor(factor);
            });

        s_Lua.set_function("Audio_GetSlotFadeDuration",
            [](uint32_t entityID, const std::string& slotId) -> float
            {
                auto& ecs = EngineFramework::GameObjectSystem::GetRegistry();
                entt::entity entity = static_cast<entt::entity>(entityID);
                if (!ecs.valid(entity) || !ecs.all_of<AudioComponent>(entity)) return 1.0f;
                auto& comp = ecs.get<AudioComponent>(entity);
                const auto* slot = comp.FindSlot(slotId);
                return slot ? slot->fadeDuration : 1.0f;
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
            },
            "GetPosition_NDC", [](UIComponent& ui) {
                return std::make_tuple(ui.position_NDC.x, ui.position_NDC.y);
            },
            "SetPosition_NDC", [](UIComponent& ui, float x, float y) {
                ui.position_NDC = glm::vec2(x, y);
            },
            "is_NDC", [](UIComponent& ui, bool v) {
                ui.is_NDC = v;
            },
            "GetSize_NDC", [](UIComponent& ui) {
                return std::make_tuple(ui.size_NDC.x, ui.size_NDC.y);
            },
            "SetSize_NDC", [](UIComponent& ui, float x, float y) {
                ui.size_NDC = glm::vec2(x, y);
            }
            
        );

        s_Lua.set_function("Script_RequestReloadEntity",
            [](uint32_t entityId)
            {
                entt::entity e = static_cast<entt::entity>(entityId);
                Engine::ScriptSystem::RequestReloadEntity(e);
            });

        s_Lua.set_function("Script_RequestReloadAll",
            []()
            {
                Engine::ScriptSystem::RequestReloadAll();
            });

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

        queryTable.set_function("GetCameraForward",
            [](uint32_t entityId)
            {
                if (!s_RuntimeRegistry)
                    return std::make_tuple(0.f, 0.f, 1.f);

                entt::entity e = static_cast<entt::entity>(entityId);
                if (!s_RuntimeRegistry->valid(e))
                    return std::make_tuple(0.f, 0.f, 1.f);

                if (!s_RuntimeRegistry->all_of<CameraComponent>(e))
                    return std::make_tuple(0.f, 0.f, 1.f);

                const auto& cam = s_RuntimeRegistry->get<CameraComponent>(e);

                glm::vec3 fwd = cam.target - cam.position;

                if (glm::length(fwd) < 0.00001f)
                    fwd = glm::vec3(0, 0, 1);

                fwd = glm::normalize(fwd);
                return std::make_tuple(fwd.x, fwd.y, fwd.z);
            });


        queryTable.set_function("IsCameraLookingAt",
            [](uint32_t cameraId, uint32_t targetId, float fovDegrees) -> bool
            {
                if (!s_RuntimeRegistry)
                    return false;

                entt::entity camE = static_cast<entt::entity>(cameraId);
                entt::entity tgtE = static_cast<entt::entity>(targetId);

                if (!s_RuntimeRegistry->valid(camE) || !s_RuntimeRegistry->valid(tgtE))
                    return false;

                if (!s_RuntimeRegistry->all_of<CameraComponent>(camE) ||
                    !s_RuntimeRegistry->all_of<TransformComponent>(tgtE))
                    return false;

                const auto& cam = s_RuntimeRegistry->get<CameraComponent>(camE);
                const auto& tgt = s_RuntimeRegistry->get<TransformComponent>(tgtE);

                glm::vec3 forward = cam.target - cam.position;
                if (glm::length(forward) < 0.00001f)
                    return false;
                forward = glm::normalize(forward);

                glm::vec3 toTarget = tgt.position - cam.position;
                if (glm::length(toTarget) < 0.00001f)
                    return true;
                toTarget = glm::normalize(toTarget);

                float cosAngle = glm::dot(forward, toTarget);

                float halfRad = glm::radians(fovDegrees * 0.5f);
                float threshold = cosf(halfRad);

                return cosAngle >= threshold;
            });


        queryTable.set_function("GetTransform",
            [](uint32_t entityId) -> TransformComponent*
            {
                if (!s_RuntimeRegistry) return nullptr;
                entt::entity e = static_cast<entt::entity>(entityId);
                if (!s_RuntimeRegistry->valid(e)) return nullptr;
                if (!s_RuntimeRegistry->all_of<TransformComponent>(e)) return nullptr;
                return &s_RuntimeRegistry->get<TransformComponent>(e);
            });

        queryTable.set_function("GetAudio",
            [](uint32_t entityId) -> AudioComponent*
            {
                if (!s_RuntimeRegistry) return nullptr;

                entt::entity e = static_cast<entt::entity>(entityId);
                if (!s_RuntimeRegistry->valid(e)) return nullptr;
                if (!s_RuntimeRegistry->all_of<AudioComponent>(e)) return nullptr;

                return &s_RuntimeRegistry->get<AudioComponent>(e);
            });

        queryTable.set_function("GetLight", [](uint32_t entityId) -> LightComponent* {
            if (!s_RuntimeRegistry) return nullptr;

            entt::entity e = static_cast<entt::entity>(entityId);
            if (!s_RuntimeRegistry->valid(e)) return nullptr;

            if (!s_RuntimeRegistry->all_of<LightComponent>(e)) return nullptr;

            return &s_RuntimeRegistry->get<LightComponent>(e);
            });

        queryTable.set_function("GetParticleEmitter", [](uint32_t entityId) -> ParticleEmitterComponent* {
            if (!s_RuntimeRegistry) return nullptr;

            entt::entity e = static_cast<entt::entity>(entityId);
            if (!s_RuntimeRegistry->valid(e)) return nullptr;

            if (!s_RuntimeRegistry->all_of<ParticleEmitterComponent>(e)) return nullptr;

            return &s_RuntimeRegistry->get<ParticleEmitterComponent>(e);
            });


        // Audio Bindings
        sol::table audioTable = s_Lua.create_named_table("Audio");

        //// Play selected clip index for entity
        //audioTable.set_function("Play",
        //    [](uint32_t entityId, int clipIndex)
        //    {
        //        if (!s_RuntimeRegistry) return;

        //        entt::entity e = static_cast<entt::entity>(entityId);
        //        if (!s_RuntimeRegistry->valid(e)) return;
        //        if (!s_RuntimeRegistry->all_of<AudioComponent>(e)) return;

        //        auto& ac = s_RuntimeRegistry->get<AudioComponent>(e);

        //        if (clipIndex < 0) return;
        //        auto* clip = ac.GetClip(static_cast<std::size_t>(clipIndex));
        //        if (!clip) return;

        //        clip->playRequested = true; // your AudioSystem will consume this
        //    });

        //// Set volume for selected clip index
        //audioTable.set_function("SetVolume",
        //    [](uint32_t entityId, int clipIndex, float v)
        //    {
        //        if (!s_RuntimeRegistry) return;

        //        entt::entity e = static_cast<entt::entity>(entityId);
        //        if (!s_RuntimeRegistry->valid(e)) return;
        //        if (!s_RuntimeRegistry->all_of<AudioComponent>(e)) return;

        //        auto& ac = s_RuntimeRegistry->get<AudioComponent>(e);

        //        if (clipIndex < 0) return;
        //        auto* clip = ac.GetClip(static_cast<std::size_t>(clipIndex));
        //        if (!clip) return;

        //        // clamp if you want
        //        if (v < 0.0f) v = 0.0f;
        //        if (v > 1.0f) v = 1.0f;

        //        clip->volume = v;
        //    });


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

        s_Lua.set_function("GetPlayerFlashlightID", []() -> sol::optional<uint32_t> {
            auto view = registry.view<Engine::LuaScriptComponent, LightComponent>();

            for (auto [e, lsc, light] : view.each()) {
                for (auto& inst : lsc.Scripts) {
                    if (inst.ScriptGuid.m_Value == 0) continue;

                    auto full_guid = xresource::full_guid(
                        inst.ScriptGuid,
                        EngineFramework::AssetManagerSystem::SCRIPT_GUID
                    );

                    if (EngineFramework::AssetManagerSystem::GetName(full_guid) == "flashlight_controller") {
                        return static_cast<uint32_t>(e); // Return the raw Entity ID!
                    }
                }
            }
            return sol::nullopt; // Returns 'nil' in Lua
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

        s_Lua.set_function("StateMachine_GetCurrentStateName", []() -> std::string
            {
                return EngineFramework::StateMachineSystem::GetInstance().get_currentState_name();
            });


        //get_prevState_name

        s_Lua.set_function("StateMachine_GetPrevStateName", []() -> std::string
            {
                return EngineFramework::StateMachineSystem::GetInstance().get_prevState_name();
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

        event_dispatcher.sink<Event_OnTriggerEnter>().connect<&ScriptSystem::OnTriggerEnter>();
        event_dispatcher.sink<Event_OnTriggerExit>().connect<&ScriptSystem::OnTriggerExit>();
        event_dispatcher.sink<Event_OnTriggerStay>().connect<&ScriptSystem::OnTriggerStay>();
        LOGI("[ScriptSystem] About to subscribe to Button_Event...");
        event_dispatcher.sink<Button_Event>().connect<&ScriptSystem::OnButtonEvent>();
        LOGI("[ScriptSystem] Successfully subscribed to Button_Event!");

        LOGI("[ScriptSystem] Subscribed to Trigger Events.");
#endif
    }

    void ScriptSystem::OnRuntimeStop(entt::registry& reg) {
        ;
        auto nativeView = reg.view<NativeScriptComponent>();
        for (auto [e, nsc] : nativeView.each()) {
            if (nsc.Instance) { nsc.Instance->OnDestroy(); DestroyNativeInstance(nsc); }
        }

#if defined(ENGINE_ENABLE_LUA)
        event_dispatcher.sink<Event_OnTriggerEnter>().disconnect<&ScriptSystem::OnTriggerEnter>();
        event_dispatcher.sink<Event_OnTriggerExit>().disconnect<&ScriptSystem::OnTriggerExit>();
        event_dispatcher.sink<Event_OnTriggerStay>().disconnect<&ScriptSystem::OnTriggerStay>();
        event_dispatcher.sink<Button_Event>().disconnect<&ScriptSystem::OnButtonEvent>();

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
            //LOGE("[LuaRuntime] ScriptAsset not found for GUID %llu (entity %u)",
                //inst.ScriptGuid.m_Value, static_cast<unsigned>(e));
            //inst.Loaded = false;
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
