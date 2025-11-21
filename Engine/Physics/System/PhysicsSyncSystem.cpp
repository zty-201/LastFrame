#define GLM_ENABLE_EXPERIMENTAL
#include "PhysicsSyncSystem.hpp"

#include <glm/gtx/quaternion.hpp>
#include <entt/entt.hpp>

#include "TransformComponent.h"
#include "PhysicsComponent.h"
#include "PhysicsMaterialComponent.h"
#include "PhysicsWorld.hpp"
#include "ColliderComponent.h"

void PhysicsSyncSystem::Update(entt::registry& ecs)
{
    auto view = ecs.view<TransformComponent, PhysicsComponent, PhysicsMaterialComponent, ColliderComponent>();

    for (auto&& [ent, transform, physics, material, collider] : view.each())
    {
        if (!PhysicsWorld::Get().HasBody(ent))
            continue;

        if (material.isStatic)
        {
            const glm::quat q_editor = glm::quat(glm::radians(transform.rotation));
            PhysicsWorld::Get().SetPositionRotation(ent, transform.position, q_editor, /*activate=*/false);
            PhysicsWorld::Get().SetLinearVelocity(ent, glm::vec3(0.0f)); // ensure no drift
            // no gravity for static bodies; skip the rest
            physics._prevUseGravity = physics.useGravity;
            continue;
        }

        if (physics.useGravity && !physics._prevUseGravity)
        {
            // Push ECS transform to Jolt so the body starts where you placed it
            glm::quat q = glm::quat(glm::radians(transform.rotation));
            PhysicsWorld::Get().SetPositionRotation(ent, transform.position, q, /*activate=*/true);

            // Clear old motion so it doesn't “remember” a velocity from before
            PhysicsWorld::Get().SetLinearVelocity(ent, glm::vec3(0.0f));
        }

        if (physics.editorOverride)
        {
            glm::quat q = glm::quat(glm::radians(transform.rotation));
            PhysicsWorld::Get().SetPositionRotation(ent, transform.position, q, true);
            PhysicsWorld::Get().SetLinearVelocity(ent, glm::vec3(0.0f));
            physics.editorOverride = false; // consume flag so it only happens once
            continue; // skip normal Jolt -> ECS pull this frame
        }

        transform.position = PhysicsWorld::Get().GetBodyPosition(ent);

        const glm::quat jq = PhysicsWorld::Get().GetBodyRotation(ent);
        transform.rotation = glm::degrees(glm::eulerAngles(jq));

        physics.velocity = PhysicsWorld::Get().GetBodyVelocity(ent);

        PhysicsWorld::Get().SetGravityFactor(
            ent, physics.useGravity ? physics.gravityFactor : 0.0f
        );

        if (physics.useGravity) 
        {
            PhysicsWorld::Get().ActivateBody(ent);      
        }
        else {
            auto v = PhysicsWorld::Get().GetBodyVelocity(ent);
            if (v.y != 0.0f) 
            {                          
                v.y = 0.0f;
                PhysicsWorld::Get().SetLinearVelocity(ent, v);
            }
        }

        physics._prevUseGravity = physics.useGravity;
    }
}