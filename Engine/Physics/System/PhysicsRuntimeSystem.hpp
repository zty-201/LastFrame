#pragma once

#include "ISystem.hpp"
#include <entt/entt.hpp>
#include "PhysicsWorld.hpp"
#include "PhysicsBuildSystem.hpp"
#include "PhysicsSyncSystem.hpp"

namespace EngineFramework
{
    class PhysicsRuntimeSystem final : public ISystem
    {
    public:
        PhysicsRuntimeSystem() = default;
        ~PhysicsRuntimeSystem() override = default;

    private:
        // Called by CoreEngine at startup
        void Initialize() override
        {
            PhysicsWorld::Get().Initialize();
        }

        // Called every frame by CoreEngine
        void Update(float delta_time) override
        {
            // 1. Make sure ECS entities have physics bodies in Jolt
            PhysicsBuildSystem::Update();

            // 2. Step the physics world forward
            PhysicsWorld::Get().StepSimulation(delta_time);

            // 3. Sync physics results (pos/rot/vel) back into ECS
            PhysicsSyncSystem::Update();
        }

        // Called after all systems update
        void PostUpdate(float /*delta_time*/) override
        {
            // (Optional future: physics debug draw, etc.)
        }
    };
}