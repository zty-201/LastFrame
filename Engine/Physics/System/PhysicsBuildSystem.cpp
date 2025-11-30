#include "PhysicsBuildSystem.hpp"
#include <entt/entt.hpp>

#include "PhysicsWorld.hpp"
#include "TransformComponent.h"
#include "PhysicsMaterialComponent.h"
#include "ColliderComponent.h"
#include "AssetManager.hpp"
#include "Mesh.h"
#include "MeshComponent.h"

//static void FitColliderToMeshAABB(entt::registry& ecs, entt::entity e)
//{
//    // Need both mesh + collider
//    if (!ecs.all_of<MeshComponent, ColliderComponent>(e))
//        return;
//
//    auto& meshComp = ecs.get<MeshComponent>(e);
//    auto& collider = ecs.get<ColliderComponent>(e);
//
//    // 1) Look up mesh resource from name
//    auto meshGuid = EngineFramework::AssetManagerSystem::GetFullGuid(
//        EngineFramework::AssetManagerSystem::MESH_GUID,
//        meshComp.meshName
//    );
//
//    if (!EngineFramework::AssetManagerSystem::HasResource(meshGuid))
//        return;
//
//    auto meshData =
//        EngineFramework::AssetManagerSystem::GetResource<Mesh::MeshData>(meshGuid);
//    if (!meshData)
//        return;
//
//    // 2) Get AABB from MeshData (object-space)
//    const auto& aabb = meshData->GetAABB();
//    glm::vec3 center = aabb.center();
//    glm::vec3 half = aabb.halfExtents();
//
//    // 3) Make sure we have at least one shape
//    if (collider.shapes.empty())
//        collider.shapes.emplace_back();
//
//    ColliderData& shape = collider.shapes[0];
//
//    // 4) Configure as a box collider that matches the mesh bounds
//    shape.type = static_cast<int>(ColliderShape::Box); // <- int, not enum
//    shape.boxHalfExtents = half;                                 // same half extents
//    shape.offsetPosition = center;                               // same local offset
//
//    // 5) Update collider AABB too (optional but nice)
//    collider.minBounds = center - half;
//    collider.maxBounds = center + half;
//}

void PhysicsBuildSystem::Update(entt::registry& ecs)
{
    //LOGI("[PhysicsBuild] Update called");

    //LOGI("[PhysicsBuild] transforms = %zu", ecs.storage<TransformComponent>().size());
    //LOGI("[PhysicsBuild] physics    = %zu", ecs.storage<PhysicsComponent>().size());
    //LOGI("[PhysicsBuild] colliders  = %zu", ecs.storage<ColliderComponent>().size());

    auto view = ecs.view<TransformComponent, PhysicsMaterialComponent, ColliderComponent, PhysicsComponent>();
    //LOGI("[PhysicsBuild] view size = %zu", view.size_hint());

    for (auto&& [ent, transform, material, collider, physics] : view.each())
    {
        //if (static_cast<ColliderComponent::Mode>(collider.mode) == ColliderComponent::Mode::LEVEL_EDITOR)
        //{
        //    if (PhysicsWorld::Get().HasBody(ent))
        //    {
        //        PhysicsWorld::Get().RemoveBody(ent);
        //    }
        //    continue; // Skip the rest of the physics update for this entity
        //}

        //LOGI("[PhysicsBuild] found entity %u", (unsigned)ent);
        const bool hasBody = PhysicsWorld::Get().HasBody(ent);
        const bool tagChanged = (material.isStatic != material._prevIsStatic);

        const float epsilon = 0.0001f;
        const bool scaleChanged = glm::distance(transform.scale, material._prevScale) > epsilon;

        if (hasBody && (tagChanged || scaleChanged))
        {
            PhysicsWorld::Get().RemoveBody(ent);
        }

        if (!hasBody || tagChanged)
        {
            //FitColliderToMeshAABB(ecs, ent);

            PhysicsWorld::Get().CreatePhysicsBody(
                ent,
                transform.position,
                transform.rotation,
                transform.scale,
                collider,
                material,
                physics.gravityFactor
            );
        }
        /*switch (static_cast<ColliderShape>(collider.shape))
        {
        case ColliderShape::Box:
        {
            LOGI("[BUILD] Creating body for entity %u (shape=%d, static=%d)",
                (uint32_t)ent,
                (int)collider.shape,
                collider.isStatic);

            PhysicsWorld::Get().CreateBoxBody(
                ent,
                transform.position,
                collider.size,
                collider.isStatic,
                collider.isTrigger
            );
            LOGI("Created a Box Body");
            break;
        }
        case ColliderShape::Sphere:
        {
            PhysicsWorld::Get().CreateSphereBody(
                ent,
                transform.position,
                collider.radius,
                collider.isStatic,
                collider.isTrigger
            );
            LOGI("Created a Sphere Body");
            break;
        }
        case ColliderShape::Capsule:
        {
            PhysicsWorld::Get().CreateCapsuleBody(
                ent,
                transform.position,
                collider.height,
                collider.capRadius,
                collider.isStatic,
                collider.isTrigger
            );
            LOGI("Created a Capsule Body");
            break;
        }
        }*/

        PhysicsWorld::Get().SetGravityFactor(
            ent, physics.useGravity ? physics.gravityFactor : 0.0f
        );

        if (!material.isStatic) {
            PhysicsWorld::Get().ActivateBody(ent);
        }

        material._prevIsStatic = material.isStatic;
    }
}