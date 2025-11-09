#include "PhysicsBuildSystem.hpp"
#include <entt/entt.hpp>

#include "PhysicsWorld.hpp"
#include "TransformComponent.h"
#include "PhysicsComponent.h"
#include "ColliderComponent.h"

void PhysicsBuildSystem::Update(entt::registry& ecs)
{
    //LOGI("[PhysicsBuild] Update called");

    //LOGI("[PhysicsBuild] transforms = %zu", ecs.storage<TransformComponent>().size());
    //LOGI("[PhysicsBuild] physics    = %zu", ecs.storage<PhysicsComponent>().size());
    //LOGI("[PhysicsBuild] colliders  = %zu", ecs.storage<ColliderComponent>().size());

    auto view = ecs.view<TransformComponent, PhysicsComponent, ColliderComponent>();
    //LOGI("[PhysicsBuild] view size = %zu", view.size_hint());

    for (auto&& [ent, transform, physics, collider] : view.each())
    {
        //LOGI("[PhysicsBuild] found entity %u", (unsigned)ent);

        if (PhysicsWorld::Get().HasBody(ent))
        {
            //LOGI("This object has a body");
            continue;
        }

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
            LOGI("Created a Box Body");
            break;
        }
        case ColliderShape::Sphere:
            PhysicsWorld::Get().CreateSphereBody(
                ent,
                transform.position,
                collider.radius,
                collider.isStatic
            );
            LOGI("Created a Sphere Body");
            break;
        case ColliderShape::Capsule:
            PhysicsWorld::Get().CreateCapsuleBody(
                ent,
                transform.position,
                collider.height,
                collider.capRadius,
                collider.isStatic
            );
            LOGI("Created a Capsule Body");
            break;
        }
    }
}