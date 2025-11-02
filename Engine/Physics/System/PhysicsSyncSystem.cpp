#define GLM_ENABLE_EXPERIMENTAL
#include "PhysicsSyncSystem.hpp"
#include <glm/gtx/quaternion.hpp>  // for glm::eulerAngles
#include <entt/entt.hpp>
#include "TransformComponent.h"
#include "PhysicsComponent.h"
#include "PhysicsWorld.hpp"

void PhysicsSyncSystem::Update()
{
    extern entt::registry registry;

    auto view = registry.view<TransformComponent, PhysicsComponent>();

    for (auto entity : view)
    {
        auto& transform = view.get<TransformComponent>(entity);
        auto& physics = view.get<PhysicsComponent>(entity);

        if (!PhysicsWorld::Get().HasBody(entity))
            continue;

        // Position straight from physics
        transform.position = PhysicsWorld::Get().GetBodyPosition(entity);

        // Rotation from physics (quat -> euler vec3)
        {
            glm::quat q = PhysicsWorld::Get().GetBodyRotation(entity);
            glm::vec3 eulerRadians = glm::eulerAngles(q); // returns pitch/yaw/roll in radians
            transform.rotation = eulerRadians;            // assumes your TransformComponent.rotation is vec3
        }

        // Velocity from physics
        physics.velocity = PhysicsWorld::Get().GetBodyVelocity(entity);
    }
}