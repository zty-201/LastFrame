#pragma once

#include "ISystem.hpp"
#include <entt/entt.hpp>

#include "PhysicsWorld.hpp"
#include "PhysicsBuildSystem.hpp"
#include "PhysicsSyncSystem.hpp"
#include "CoreEngine.hpp"

namespace EngineFramework
{
    class PhysicsRuntimeSystem final : public ISystem
    {
    public:
        PhysicsRuntimeSystem() = default;
        ~PhysicsRuntimeSystem()
        {
            PhysicsWorld::Get().Shutdown();
        }


    private:
        void Initialize() override
        {
            // init Jolt
            PhysicsWorld::Get().Initialize();
        }

        void Update(float delta_time) override
        {
            // 1) get ECS registry from the engine singleton
            // name it ecs, not registry, to avoid shadowing the global
            entt::registry& ecs = CoreEngine::GetInstance().GetRegistry();

            // 2) make sure ECS entities have Jolt bodies
            m_buildSystem.Update(ecs);

            // 3) step physics
            PhysicsWorld::Get().StepSimulation(delta_time);

            // 4) sync physics back to ECS
            m_syncSystem.Update(ecs);
        }

        void PostUpdate(float) override
        {
            // optional: physics debug draw
        }

    private:
        PhysicsBuildSystem m_buildSystem;
        PhysicsSyncSystem  m_syncSystem;
    };
}