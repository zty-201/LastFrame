#include "PhysicsBuildSystem.hpp"
#include <entt/entt.hpp>
#include "TransformComponent.h"
#include "PhysicsComponent.h"
#include "ColliderComponent.h"
#include "PhysicsWorld.hpp"

void PhysicsBuildSystem::Update()
{
    // Use the global registry defined elsewhere in the engine
    extern entt::registry registry;

    auto view = registry.view<TransformComponent, PhysicsComponent, ColliderComponent>();

    for (auto entity : view)
    {
        auto& transform = view.get<TransformComponent>(entity);
        auto& physics = view.get<PhysicsComponent>(entity);
        auto& collider = view.get<ColliderComponent>(entity);

        // If this entity already has a physics body in Jolt, skip
        if (PhysicsWorld::Get().HasBody(entity))
            continue;

        switch (collider.shapeType)
        {
        case ColliderShapeType::Box:
            PhysicsWorld::Get().CreateBoxBody(
                entity,
                transform.position,
                collider.halfExtents * 2.0f, // we store half extents, Jolt create wants full size
                physics.isStatic
            );
            break;

        case ColliderShapeType::Sphere:
            PhysicsWorld::Get().CreateSphereBody(
                entity,
                transform.position,
                collider.radius,
                physics.isStatic
            );
            break;

        case ColliderShapeType::Capsule:
            PhysicsWorld::Get().CreateCapsuleBody(
                entity,
                transform.position,
                collider.capsuleHalfHeight * 2.0f,
                collider.capsuleRadius,
                physics.isStatic
            );
            break;

        default:
            break;
        }
    }
}