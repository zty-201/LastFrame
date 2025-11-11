#define GLM_ENABLE_EXPERIMENTAL
#include "PhysicsSyncSystem.hpp"

#include <glm/gtx/quaternion.hpp>
#include <entt/entt.hpp>

#include "TransformComponent.h"
#include "PhysicsComponent.h"
#include "PhysicsWorld.hpp"

void PhysicsSyncSystem::Update(entt::registry& ecs)
{
    auto view = ecs.view<TransformComponent, PhysicsComponent>();

    for (auto&& [ent, transform, physics] : view.each())
    {
        if (!PhysicsWorld::Get().HasBody(ent))
            continue;

        if (physics.useGravity && !physics._prevUseGravity)
        {
            // Push ECS transform to Jolt so the body starts where you placed it
            glm::quat q = glm::quat(transform.rotation); // your Transform stores Euler
            PhysicsWorld::Get().SetPositionRotation(ent, transform.position, q, /*activate=*/true);

            // Clear old motion so it doesn't “remember” a velocity from before
            PhysicsWorld::Get().SetLinearVelocity(ent, glm::vec3(0.0f));
        }

        transform.position = PhysicsWorld::Get().GetBodyPosition(ent);

        glm::quat q = PhysicsWorld::Get().GetBodyRotation(ent);
        transform.rotation = glm::eulerAngles(q);

        physics.velocity = PhysicsWorld::Get().GetBodyVelocity(ent);

        PhysicsWorld::Get().SetGravityFactor(
            ent, physics.useGravity ? physics.gravityFactor : 0.0f
        );

        if (physics.useGravity) {
            PhysicsWorld::Get().ActivateBody(ent);      
        }
        else {
            auto v = PhysicsWorld::Get().GetBodyVelocity(ent);
            if (v.y != 0.0f) {                          
                v.y = 0.0f;
                PhysicsWorld::Get().SetLinearVelocity(ent, v);
            }
        }

        physics._prevUseGravity = physics.useGravity;
    }
}