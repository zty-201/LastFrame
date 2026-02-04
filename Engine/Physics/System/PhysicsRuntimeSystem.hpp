#pragma once

#include "ISystem.hpp"
#include <entt/entt.hpp>

#include "PhysicsWorld.hpp"
#include "PhysicsBuildSystem.hpp"
#include "PhysicsSyncSystem.hpp"
#include "CoreEngine.hpp"
#include "Screen.hpp"
#include "Level_Editor.hpp"

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
            //entt::registry& ecs = CoreEngine::GetInstance().GetRegistry();

            /*if (!Screen::GetInstance().is_simulator_running)
                return;*/

            bool shouldRunPhysics = true;
            bool editorActive = false;

#if defined(PLATFORM_WINDOWS) && defined(_DEBUG)
            if (LevelEditor::GetInstance().Get_Editor_State())
            {
                editorActive = true;
                shouldRunPhysics = Screen::GetInstance().is_simulator_running;
            }
#endif

            if (editorActive)
            {
                static bool wasRunning = false;

                if (!shouldRunPhysics && !wasRunning)
                {
                    PhysicsWorld::Get().DestroyAllBodies();
                }
                wasRunning = shouldRunPhysics;
            }

            if (!shouldRunPhysics) return;

            // 2) make sure ECS entities have Jolt bodies
            m_buildSystem.Update(registry);

            auto charView = registry.view<CharacterControllerComponent, TransformComponent>();
            for (auto [entity, character, transform] : charView.each())
            {
                // Optional: Sync runtime settings if you want to tweak them live
                PhysicsWorld::Get().SetCharacterSettings(entity, character.mass, character.maxSlopeAngle);

                // Update Position & Velocity via Jolt
                glm::vec3 newPos = PhysicsWorld::Get().UpdateCharacter(
                    entity,
                    character.controlVelocity, // Input from Lua scripts
                    delta_time,
                    character.gravityFactor,
                    // Outputs
                    character.isGrounded,
                    character.groundNormal,
                    character.controlVelocity
                );
                transform.position = newPos;
            }

            // 3) step physics
            PhysicsWorld::Get().StepSimulation(delta_time);

            // 4) sync physics back to ECS
            m_syncSystem.Update(registry);
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