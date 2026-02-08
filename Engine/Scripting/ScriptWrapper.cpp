#include "pch.hpp"
#include "ScriptWrapper.hpp"

#if defined(ENGINE_ENABLE_LUA) && defined(DEBUG)
#include "Scripting/ScriptDebug.hpp"
#endif

// IMPORTANT:
// This must refer to an actual definition somewhere (exactly one .cpp file should contain: entt::registry registry; )
extern entt::registry registry;

namespace EngineFramework
{
    ScriptWrapper::ScriptWrapper()
    {
        ScriptWrapperPtr = this;
    }

    ScriptWrapper::~ScriptWrapper()
    {
        // Shutdown order: stop runtime first (calls OnDestroy), then shutdown lua state/resources
        if (m_RuntimeStarted)
        {
            Engine::ScriptSystem::OnRuntimeStop(registry);
            m_RuntimeStarted = false;
        }

#if defined(ENGINE_ENABLE_LUA) && defined(DEBUG)
        // If ScriptDebug has Shutdown and you want it tied to lifetime:
        Engine::ScriptDebug::Shutdown();
#endif

        if (m_Initialized)
        {
            Engine::ScriptSystem::Shutdown();
            m_Initialized = false;
        }
    }

    void ScriptWrapper::Initialize()
    {
        // Init global scripting state (lua state, bindings, event subscriptions, etc.)
        Engine::ScriptSystem::Init();
        m_Initialized = true;

        // Start the runtime (load scripts for all entities with script components)
        Engine::ScriptSystem::OnRuntimeStart(registry);
        m_RuntimeStarted = true;

#if defined(ENGINE_ENABLE_LUA) && defined(DEBUG)
        Engine::ScriptDebug::Initialize();
#endif
    }

    void ScriptWrapper::Update(float delta_time)
    {
        if (!m_Initialized || !m_RuntimeStarted)
            return;

        Engine::ScriptSystem::OnUpdate(registry, delta_time);

#if defined(ENGINE_ENABLE_LUA) && defined(DEBUG)
        Engine::ScriptDebug::Update(delta_time);
#endif
    }
}
