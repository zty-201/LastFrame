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

        transform.position = PhysicsWorld::Get().GetBodyPosition(ent);

        glm::quat q = PhysicsWorld::Get().GetBodyRotation(ent);
        transform.rotation = glm::eulerAngles(q);

        physics.velocity = PhysicsWorld::Get().GetBodyVelocity(ent);
    }
}