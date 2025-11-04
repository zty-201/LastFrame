/*
*****************************************************************/
/*!
\file   Script.hpp
\author Teng Jun Rong Jerron
\par    t.junrongjerron@digipen.edu
\date   September 2025
\brief
This file contains the definitions of the functions that are needed for
handling the Scripts (multi-script version).
Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* *******************************************************************/

#pragma once
#include <entt/entt.hpp>
#include <filesystem>
#include <string>
#include <vector>

#if defined(ENGINE_ENABLE_LUA)
#include <sol/sol.hpp>
#endif

namespace Engine {

    class ScriptableEntity {
    public:
        virtual ~ScriptableEntity() = default;
        virtual void OnCreate() {}
        virtual void OnUpdate(float) {}
        virtual void OnDestroy() {}
        void SetEntity(entt::entity e, entt::registry* reg) { m_Entity = e; m_Registry = reg; }

    protected:
        template<typename T, typename... Args>
        T& AddComponent(Args&&... args) { return m_Registry->emplace<T>(m_Entity, std::forward<Args>(args)...); }
        template<typename T>
        T& GetComponent() { return m_Registry->get<T>(m_Entity); }
        template<typename T>
        bool HasComponent() const { return m_Registry->any_of<T>(m_Entity); }
        template<typename T>
        void RemoveComponent() { m_Registry->remove<T>(m_Entity); }

    protected:
        entt::entity    m_Entity{ entt::null };
        entt::registry* m_Registry{ nullptr };
    };

    struct NativeScriptComponent {
        using InstantiateFn = ScriptableEntity * (*)();
        using DestroyFn = void(*)(NativeScriptComponent&);
        ScriptableEntity* Instance{ nullptr };
        InstantiateFn     Instantiate{ nullptr };
        DestroyFn         Destroy{ nullptr };
        bool              Enabled{ true };

        template<typename T>
        void Bind() {
            Instantiate = []() -> ScriptableEntity* { return new T(); };
            Destroy = [](NativeScriptComponent& nsc) { delete nsc.Instance; nsc.Instance = nullptr; };
        }
    };

    // ----------------------------------------------------------------
    // Multi-script Lua component
    // ----------------------------------------------------------------
    struct LuaScriptComponent {
        bool Enabled{ true };

#if defined(ENGINE_ENABLE_LUA)
        struct LuaInstance {
            xresource::instance_guid ScriptGuid; // Which script this instance runs
            sol::environment Env{ sol::nil };
            sol::table       Self{ sol::nil };
            sol::function    FnOnCreate{ sol::nil };
            sol::function    FnOnUpdate{ sol::nil };
            sol::function    FnOnDestroy{ sol::nil };
            bool             Loaded{ false };
        };

        std::vector<LuaInstance> Scripts; // Multiple script instances per entity
#else
        std::vector<xresource::instance_guid> ScriptGuids; // Serialized in editor
#endif
    };

    // ----------------------------------------------------------------
    // Script System
    // ----------------------------------------------------------------
    class ScriptSystem {
    public:
        static void Init();
        static void Shutdown();

        static void OnRuntimeStart(entt::registry& reg);
        static void OnRuntimeStop(entt::registry& reg);
        static void OnUpdate(entt::registry& reg, float dt);
        static void ReloadScript(entt::registry& reg, entt::entity e);

#if defined(ENGINE_ENABLE_LUA)
        static sol::state& GetLuaState();
#endif

    private:
        static void EnsureNativeInstance(entt::registry& reg, entt::entity e, NativeScriptComponent& nsc);
        static void DestroyNativeInstance(NativeScriptComponent& nsc);

#if defined(ENGINE_ENABLE_LUA)
        static void LoadLuaFor(entt::registry& reg, entt::entity e, LuaScriptComponent::LuaInstance& inst);
        static void UnloadLuaFor(LuaScriptComponent::LuaInstance& inst, bool callOnDestroy);
#endif
    };

} // namespace Engine
