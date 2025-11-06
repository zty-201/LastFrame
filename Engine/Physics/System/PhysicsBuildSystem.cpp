#include "PhysicsBuildSystem.hpp"
#include <entt/entt.hpp>

#include "PhysicsWorld.hpp"
#include "TransformComponent.h"
#include "PhysicsComponent.h"
#include "ColliderComponent.h"

void PhysicsBuildSystem::Update(entt::registry& ecs)
{
    auto view = ecs.view<TransformComponent, PhysicsComponent, ColliderComponent>();

    for (auto&& [ent, transform, physics, collider] : view.each())
    {
        if (PhysicsWorld::Get().HasBody(ent))
            continue;

        ColliderShape shape = static_cast<ColliderShape>(collider.shape);

        switch (shape)
        {
        case ColliderShape::Box:
        {
            glm::vec3 half = collider.size * 0.5f;
            PhysicsWorld::Get().CreateBoxBody(
                ent,
                transform.position,
                half * 2.0f,
                collider.isStatic
            );
            break;
        }
        case ColliderShape::Sphere:
            PhysicsWorld::Get().CreateSphereBody(
                ent,
                transform.position,
                collider.radius,
                collider.isStatic
            );
            break;
        case ColliderShape::Capsule:
            PhysicsWorld::Get().CreateCapsuleBody(
                ent,
                transform.position,
                collider.height,
                collider.capRadius,
                collider.isStatic
            );
            break;
        }
    }
}