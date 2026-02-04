#include "PhysicsWorld.hpp"

#include <Jolt/Core/Factory.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/CompoundShape.h>
#include <Jolt/Core/Reference.h>
#include <Jolt/Physics/Collision/Shape/Shape.h> // Base Shape definition
#include <Jolt/Physics/Collision/Shape/StaticCompoundShape.h>
#include <Jolt/Physics/Body/MassProperties.h>
#include <cassert>
#include <iostream>
#include "TransformComponent.h"
#include "ColliderComponent.h"
#include "TriggerBoxComponent.h"
#include "PhysicsMaterialComponent.h"
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

//TEST VALUES ONLY
const float TEST_DENSITY = 1000.0f;
const float FULL_GRAVITY = 1.0f;

namespace JPH
{
    const float cPi = 3.14159265358979323846f;
}

// ------------------------------------------------------------
// 5. Singleton accessor
// ------------------------------------------------------------
PhysicsWorld& PhysicsWorld::Get()
{
    static PhysicsWorld instance;
    return instance;
}

// ------------------------------------------------------------
// 6. Initialize()
// ------------------------------------------------------------
void PhysicsWorld::Initialize()
{
    // 6.1 Register allocators and factory
    JPH::RegisterDefaultAllocator();
    JPH::Factory::sInstance = new JPH::Factory();
    JPH::RegisterTypes();

    // 6.2 Create JobSystem (thread pool)
    //    4 worker threads for now. You can tune this.
    m_jobSystem = new JPH::JobSystemThreadPool(
        JPH::cMaxPhysicsJobs,
        JPH::cMaxPhysicsBarriers,
        4
    );

    m_tempAllocator = std::make_unique<JPH::TempAllocatorImpl>(10 * 1024 * 1024);

    // 6.3 Create filter objects
    m_broadPhaseLayerInterface = std::make_unique<BroadPhaseLayerInterfaceImpl>();
    m_objectVsBroadPhaseLayerFilter = std::make_unique<ObjectVsBroadPhaseLayerFilterImpl>();
    m_objectLayerPairFilter = std::make_unique<ObjectLayerPairFilterImpl>();

    // 6.4 Physics system capacity tuning
    const uint32_t cMaxBodies = 1024;
    const uint32_t cNumBodyMutexes = 0;     // 0 = let Jolt decide
    const uint32_t cMaxBodyPairs = 1024;
    const uint32_t cMaxContactConstraints = 1024;

    // 6.5 Create PhysicsSystem and init
    m_physicsSystem = new JPH::PhysicsSystem();
    m_physicsSystem->Init(
        cMaxBodies,
        cNumBodyMutexes,
        cMaxBodyPairs,
        cMaxContactConstraints,
        *m_broadPhaseLayerInterface,
        *m_objectVsBroadPhaseLayerFilter,
        *m_objectLayerPairFilter
    );

    // 6.6 Set gravity (Y down)
    m_physicsSystem->SetGravity(JPH::Vec3(0.0f, -9.81f, 0.0f));

    m_physicsSystem->SetContactListener(&m_contactListener);
   
    // Reset step accumulator
    m_accumulator = 0.0f;

    LOGI("[PhysicsWorld] Initialized Jolt Physics");

    // --------------------------------------------------
    // TEMP: debug bodies to verify contact listener
    // --------------------------------------------------
    {
        /*entt::entity eA = (entt::entity)1;
        entt::entity eB = (entt::entity)2;

        glm::vec3 pos(0.0f, 0.0f, 0.0f);
        glm::vec3 size(1.0f, 1.0f, 1.0f);*/

        // A = dynamic, B = static
        //CreateBoxBody(eA, pos, size, TEST_DENSITY, FULL_GRAVITY, /*isStatic=*/false, /*isTrigger=*/false);
        //CreateBoxBody(eB, pos, size, TEST_DENSITY, FULL_GRAVITY, /*isStatic=*/true,  /*isTrigger=*/false);
    }
}

// ------------------------------------------------------------
// 7. Shutdown()
// ------------------------------------------------------------
void PhysicsWorld::Shutdown()
{
    // Destroy job system
    if (m_jobSystem)
    {
        delete m_jobSystem;
        m_jobSystem = nullptr;
    }

    // Destroy physics system
    if (m_physicsSystem)
    {
        //JPH::BodyInterface& bodyInterface = m_physicsSystem->GetBodyInterfaceNoLock();

        //// 1a. Copy entity IDs and get Jolt BodyIDs
        //std::vector<JPH::BodyID> bodies_to_destroy;
        //for (auto const& pair : m_entityToBody)
        //{
        //    bodies_to_destroy.push_back(pair.second);
        //}

        //if (!bodies_to_destroy.empty())
        //{
        //    // 1b. Destroy all remaining bodies in Jolt
        //    bodyInterface.RemoveBodies(bodies_to_destroy.data(), (int)bodies_to_destroy.size());
        //    bodyInterface.DestroyBodies(bodies_to_destroy.data(), (int)bodies_to_destroy.size());
        //}

        //// 1c. Clear local maps (Now empty in Jolt)
        //m_entityToBody.clear();
        //m_bodyToEntity.clear();

        // 2. DELETE THE JOLT WORLD
        delete m_physicsSystem;
        m_physicsSystem = nullptr;
    }

    JPH::UnregisterTypes();

    // Unregister types / factory
    if (JPH::Factory::sInstance)
    {
        delete JPH::Factory::sInstance;
        JPH::Factory::sInstance = nullptr;
    }

    LOGI("[PhysicsWorld] Shutdown complete");
}

// ------------------------------------------------------------
// 8. StepSimulation(dt)
//    We'll hook in your fixed-step accumulator logic next stage.
// ------------------------------------------------------------
void PhysicsWorld::StepSimulation(float dt)
{
    // Accumulate time passed from the engine
    m_accumulator += dt;

    // Run fixed steps of size m_fixedDt (e.g. 1/60 sec)
    while (m_accumulator >= m_fixedDt)
    {
        if (m_physicsSystem && m_jobSystem)
        {
            // Create a temporary allocator for this step.
            // 10 * 1024 is 10 KB scratch. You can tune this if needed.
            //JPH::TempAllocatorImpl temp_allocator(10 * 1024);

            // Advance the simulation by one fixed tick.
            // collisionSteps = 1 is fine for now.
            // We ignore the return value (EPhysicsUpdateError).
            (void)m_physicsSystem->Update(
                m_fixedDt,
                1,                  // collision steps
                m_tempAllocator.get(),    // temp allocator for this step
                m_jobSystem         // job system you created in Initialize()
            );
        }

        // Consume one fixed tick worth of accumulated time
        m_accumulator -= m_fixedDt;
    }
}

// ------------------------------------------------------------
// 9. Body management stubs
//    We'll implement these in the next stage (BuildSystem).
// ------------------------------------------------------------
JPH::ShapeRefC CreateJoltShapeFromCollider(const ColliderComponent& collider, const glm::vec3& scale)
{
    if (collider.shapes.empty())
    {
        LOGE("[PhysicsWorld] Cannot create shape: ColliderComponent has no sub-shapes.");
        return JPH::ShapeRefC();
    }

    JPH::StaticCompoundShapeSettings compoundSettings;

    const float MIN_DIMENSION = 0.001f;

    glm::vec3 absScale = glm::abs(scale);

    for (const auto& shapeData : collider.shapes)
    {
        JPH::ShapeRefC singleShape;

        switch (static_cast<ColliderShape>(shapeData.type))
        {
            case ColliderShape::Box:
            {
                // 1. Get Base
                glm::vec3 halfExt = shapeData.boxHalfExtents;

                // 2. Apply Scale
                halfExt *= absScale;

                // 3. SANITIZE (Handle NaN and Zero)
                // If scale is 0, this becomes 0. If scale is uninitialized, might be garbage.
                // We force it to be at least 0.05f (5cm) to be safe.
                float safeMin = 0.05f;

                halfExt.x = (std::isnan(halfExt.x) || halfExt.x < safeMin) ? safeMin : halfExt.x;
                halfExt.y = (std::isnan(halfExt.y) || halfExt.y < safeMin) ? safeMin : halfExt.y;
                halfExt.z = (std::isnan(halfExt.z) || halfExt.z < safeMin) ? safeMin : halfExt.z;

                // 4. Create
                singleShape = new JPH::BoxShape(JPH::Vec3(halfExt.x, halfExt.y, halfExt.z));
                break;
            }
            case ColliderShape::Sphere:
            {
                float maxScale = std::max({ absScale.x, absScale.y, absScale.z });
                float finalRadius = std::max(shapeData.sphereRadius * maxScale, MIN_DIMENSION);

                singleShape = new JPH::SphereShape(finalRadius);
                break;
            }
            case ColliderShape::Capsule:
            {
                float scaleY = absScale.y;
                float scaleXZ = std::max(absScale.x, absScale.z);

                float finalHeight = shapeData.capsuleHeight * scaleY;
                float finalRadius = std::max(shapeData.capsuleRadius * scaleXZ, MIN_DIMENSION);

                // Recalculate cylinder half-height (Total Height - 2 * Radius) / 2
                float halfCylinderHeight = 0.5f * (finalHeight - 2.0f * finalRadius);
                if (halfCylinderHeight < 0.0f) halfCylinderHeight = MIN_DIMENSION; // Ensure valid geometry

                singleShape = new JPH::CapsuleShape(halfCylinderHeight, finalRadius);
                break;
                break;
            }
        }

        if (singleShape)
        {
            glm::quat glmRotation = glm::quat(glm::radians(shapeData.offsetRotation));

            glm::vec3 scaledOffset = shapeData.offsetPosition * scale;

            compoundSettings.AddShape(
                JPH::Vec3(scaledOffset.x, scaledOffset.y, scaledOffset.z),
                JPH::Quat(glmRotation.x, glmRotation.y, glmRotation.z, glmRotation.w),
                singleShape
            );
        }
    }

    JPH::ShapeSettings::ShapeResult shapeResult = compoundSettings.Create();

    if (shapeResult.HasError()) {
        LOGE("[JOLT] Failed to finalize compound shape: %s", shapeResult.GetError().c_str());
        return JPH::ShapeRefC();
    }

    return shapeResult.Get();
}

void PhysicsWorld::CreatePhysicsBody(entt::entity entity, const glm::vec3& pos,  const glm::vec3& rot, const glm::vec3& scale,
    const ColliderComponent& collider, const PhysicsMaterialComponent& material, const float gravityFactor)
{
    if (HasBody(entity)) return;

    if (collider.shapes.empty()) {
        //LOGI("[JOLT] Entity %u has ColliderComponent but no shapes defined. Skipping body creation.", (unsigned)entity);
        return;
    }

    JPH::ShapeRefC finalShape = CreateJoltShapeFromCollider(collider, scale);
    if (!finalShape) return;

    JPH::EMotionType motionType;
    JPH::ObjectLayer objectLayer;
    ColliderComponent::Mode modeEnum = static_cast<ColliderComponent::Mode>(collider.mode);

    if (modeEnum == ColliderComponent::Mode::LEVEL_EDITOR)
    {
        motionType = JPH::EMotionType::Static;
        objectLayer = Layers::EDITOR_GHOST;
    }
    else
    {
        motionType = material.isStatic ? JPH::EMotionType::Static : JPH::EMotionType::Dynamic;
        objectLayer = GetObjectLayerForIsStatic(material.isStatic);
    }

    const bool isTrigger = static_cast<ColliderComponent::Mode>(collider.mode) == ColliderComponent::Mode::TRIGGER_SENSOR;

    glm::quat qRot = glm::quat(glm::radians(rot));
    JPH::Vec3 com = finalShape->GetCenterOfMass();
    glm::vec3 centerOfMassOffset = glm::vec3(com.GetX(), com.GetY(), com.GetZ());
    glm::vec3 finalBodyPos = pos + (qRot * centerOfMassOffset);

    JPH::BodyCreationSettings bodySettings(
        finalShape,
        JPH::Vec3(finalBodyPos.x, finalBodyPos.y, finalBodyPos.z),
        JPH::Quat(qRot.x, qRot.y, qRot.z, qRot.w),
        motionType,
        objectLayer
    );

    bodySettings.mIsSensor = isTrigger;

    if (motionType == JPH::EMotionType::Dynamic)
    {
        bodySettings.mMotionQuality = JPH::EMotionQuality::LinearCast;

        JPH::MassProperties massProperties;
        JPH::MassProperties shapeMassProps = finalShape->GetMassProperties();

        float volume = shapeMassProps.mMass;

        if (volume > 0.0001f)
        {
            float mass = volume * material.density;
            float mass_ratio = mass / volume;

            massProperties.mMass = mass;
            massProperties.mInertia = shapeMassProps.mInertia * mass_ratio;

            bodySettings.mMassPropertiesOverride = massProperties;
        }
        else
        {
            bodySettings.mOverrideMassProperties = JPH::EOverrideMassProperties::MassAndInertiaProvided;
            bodySettings.mMassPropertiesOverride.mMass = 1.0f;
            bodySettings.mMassPropertiesOverride.mInertia = JPH::Mat44::sIdentity();
        }

        bodySettings.mGravityFactor = gravityFactor;
        bodySettings.mFriction = material.friction;
        bodySettings.mRestitution = material.restitution;
    }

    JPH::BodyInterface& bodyInterface = m_physicsSystem->GetBodyInterfaceNoLock();
    JPH::Body* body = bodyInterface.CreateBody(bodySettings);
    if (!body) return;

    body->SetUserData(static_cast<uint64_t>(static_cast<uint32_t>(entity)));
    bodyInterface.AddBody(body->GetID(), motionType == JPH::EMotionType::Static ? JPH::EActivation::DontActivate : JPH::EActivation::Activate);

    m_entityToBody[entity] = body->GetID();
    m_bodyToEntity[body->GetID()] = entity;
}
// ------------------------------
// Box
// ------------------------------
//void PhysicsWorld::CreateBoxBody(entt::entity entity,
//    const glm::vec3& position,
//    const glm::vec3& size,   // FULL extents from your ColliderComponent
//    const float density,
//    const float gravityFactor,
//    bool isStatic,
//    bool isTrigger)
//{
//    if (HasBody(entity)) return;
//
//    // Jolt wants half extents
//    const JPH::Vec3 halfExtent(0.5f * size.x, 0.5f * size.y, 0.5f * size.z);
//    JPH::Ref<JPH::BoxShape> shape = new JPH::BoxShape(halfExtent);
//
//    const JPH::Vec3 jPos(position.x, position.y, position.z);
//    const JPH::Quat jRot = JPH::Quat::sIdentity();
//
//    const JPH::EMotionType motionType = isStatic
//        ? JPH::EMotionType::Static
//        : JPH::EMotionType::Dynamic;
//
//    JPH::BodyCreationSettings bodySettings(
//        shape, jPos, jRot, motionType, GetObjectLayerForIsStatic(isStatic)
//    );
//    bodySettings.mIsSensor = isTrigger; // <?make it a trigger/sensor if requested
//
//    if (!isStatic)
//    {
//        const float volume = size.x * size.y * size.z;
//        const float mass = volume * density;
//
//        JPH::MassProperties massProperties;
//        massProperties.SetMassAndInertiaOfSolidBox(halfExtent, mass);
//
//        bodySettings.mMassPropertiesOverride = massProperties;
//        bodySettings.mGravityFactor = gravityFactor;
//    }
//
//    JPH::BodyInterface& bodyInterface = m_physicsSystem->GetBodyInterfaceNoLock();
//    JPH::Body* body = bodyInterface.CreateBody(bodySettings);
//    if (!body) return;
//
//    body->SetUserData(static_cast<uint64_t>(static_cast<uint32_t>(entity)));
//
//    bodyInterface.AddBody(
//        body->GetID(),
//        isStatic ? JPH::EActivation::DontActivate : JPH::EActivation::Activate
//    );
//
//    m_entityToBody[entity] = body->GetID();
//    m_bodyToEntity[body->GetID()] = entity;
//}
//
//// ------------------------------
//// Sphere
//// ------------------------------
//void PhysicsWorld::CreateSphereBody(entt::entity entity,
//    const glm::vec3& position,
//    float radius,
//    const float density,
//    const float gravityFactor,
//    bool isStatic,
//    bool isTrigger)
//{
//    if (HasBody(entity)) return;
//
//    JPH::Ref<JPH::SphereShape> shape = new JPH::SphereShape(radius);
//
//    const JPH::Vec3 jPos(position.x, position.y, position.z);
//    const JPH::Quat jRot = JPH::Quat::sIdentity();
//
//    const JPH::EMotionType motionType = isStatic
//        ? JPH::EMotionType::Static
//        : JPH::EMotionType::Dynamic;
//
//    JPH::BodyCreationSettings bodySettings(
//        shape, jPos, jRot, motionType, GetObjectLayerForIsStatic(isStatic)
//    );
//    bodySettings.mIsSensor = isTrigger;
//
//    if (!isStatic)
//    {
//        const float volume = (4.0f / 3.0f) * JPH::cPi * radius * radius * radius; //4/3 * PI * r^3
//        const float mass = volume * density;
//
//        JPH::MassProperties massProperties;
//        massProperties.mMass = mass;
//        float moment = 0.4f * mass * radius * radius; //2/5 * mass * r^2
//        massProperties.mInertia = JPH::Mat44::sScale(moment);
//
//        bodySettings.mMassPropertiesOverride = massProperties;
//        bodySettings.mGravityFactor = gravityFactor;
//    }
//
//    JPH::BodyInterface& bodyInterface = m_physicsSystem->GetBodyInterfaceNoLock();
//    JPH::Body* body = bodyInterface.CreateBody(bodySettings);
//    if (!body) return;
//
//    body->SetUserData(static_cast<uint64_t>(static_cast<uint32_t>(entity)));
//
//    bodyInterface.AddBody(
//        body->GetID(),
//        isStatic ? JPH::EActivation::DontActivate : JPH::EActivation::Activate
//    );
//
//    m_entityToBody[entity] = body->GetID();
//    m_bodyToEntity[body->GetID()] = entity;
//}
//
//// ------------------------------
//// Capsule
//// ------------------------------
//void PhysicsWorld::CreateCapsuleBody(entt::entity entity,
//    const glm::vec3& position,
//    float height,   // your ColliderComponent stores full height
//    float radius,
//    const float density,
//    const float gravityFactor,
//    bool isStatic,
//    bool isTrigger)
//{
//    if (HasBody(entity)) return;
//
//    // Jolt's CapsuleShape ctor takes half-height of the cylinder part
//    JPH::Ref<JPH::CapsuleShape> shape = new JPH::CapsuleShape(0.5f * height, radius);
//
//    const JPH::Vec3 jPos(position.x, position.y, position.z);
//    const JPH::Quat jRot = JPH::Quat::sIdentity();
//
//    const JPH::EMotionType motionType = isStatic
//        ? JPH::EMotionType::Static
//        : JPH::EMotionType::Dynamic;
//
//    JPH::BodyCreationSettings bodySettings(
//        shape, jPos, jRot, motionType, GetObjectLayerForIsStatic(isStatic)
//    );
//    bodySettings.mIsSensor = isTrigger;
//
//    if (!isStatic)
//    {
//        const float cylinderHalfHeight = 0.5f * (height - 2.0f * radius);
//
//        const float sphereVolume = (4.0f / 3.0f) * JPH::cPi * radius * radius * radius;
//        const float cylinderVolume = JPH::cPi * radius * radius * (2.0f * cylinderHalfHeight);
//        const float volume = sphereVolume * cylinderVolume;
//        const float mass = volume * density;
//
//        JPH::MassProperties massProperties;
//        float axialMoment = 0.5f * mass * radius * radius;
//        massProperties.mInertia = JPH::Mat44::sScale(axialMoment);
//
//        bodySettings.mMassPropertiesOverride = massProperties;
//        bodySettings.mGravityFactor = gravityFactor;
//    }
//
//    JPH::BodyInterface& bodyInterface = m_physicsSystem->GetBodyInterfaceNoLock();
//    JPH::Body* body = bodyInterface.CreateBody(bodySettings);
//    if (!body) return;
//
//    body->SetUserData(static_cast<uint64_t>(static_cast<uint32_t>(entity)));
//
//    bodyInterface.AddBody(
//        body->GetID(),
//        isStatic ? JPH::EActivation::DontActivate : JPH::EActivation::Activate
//    );
//
//    m_entityToBody[entity] = body->GetID();
//    m_bodyToEntity[body->GetID()] = entity;
//}

void PhysicsWorld::SetGravityFactor(entt::entity entity, float factor)
{
    auto it = m_entityToBody.find(entity);
    if (it == m_entityToBody.end()) return;
    m_physicsSystem->GetBodyInterfaceNoLock().SetGravityFactor(it->second, factor);
}

void PhysicsWorld::SetPosition(entt::entity e, const glm::vec3& p, bool activate) {
    auto it = m_entityToBody.find(e); if (it == m_entityToBody.end()) return;
    m_physicsSystem->GetBodyInterfaceNoLock()
        .SetPosition(it->second, JPH::Vec3(p.x, p.y, p.z),
            activate ? JPH::EActivation::Activate : JPH::EActivation::DontActivate);
}

void PhysicsWorld::SetRotation(entt::entity e, const glm::quat& q, bool activate) {
    auto it = m_entityToBody.find(e); if (it == m_entityToBody.end()) return;
    JPH::Quat jq(q.x, q.y, q.z, q.w); // glm(w,x,y,z) vs Jolt(x,y,z,w)
    m_physicsSystem->GetBodyInterfaceNoLock()
        .SetRotation(it->second, jq,
            activate ? JPH::EActivation::Activate : JPH::EActivation::DontActivate);
}

void PhysicsWorld::SetPositionRotation(entt::entity e, const glm::vec3& p, const glm::quat& q, bool activate) {
    auto it = m_entityToBody.find(e); if (it == m_entityToBody.end()) return;
    JPH::Quat jq(q.x, q.y, q.z, q.w);
    m_physicsSystem->GetBodyInterfaceNoLock()
        .SetPositionAndRotation(it->second, JPH::Vec3(p.x, p.y, p.z), jq,
            activate ? JPH::EActivation::Activate : JPH::EActivation::DontActivate);
}

void PhysicsWorld::ActivateBody(entt::entity e)
{
    auto it = m_entityToBody.find(e);
    if (it == m_entityToBody.end()) return;
    m_physicsSystem->GetBodyInterfaceNoLock().ActivateBody(it->second);
}

void PhysicsWorld::RemoveBody(entt::entity entity)
{
    auto it = m_entityToBody.find(entity);
    if (it == m_entityToBody.end())
        return;

    JPH::BodyID bodyID = it->second;

    JPH::BodyInterface& bodyInterface = m_physicsSystem->GetBodyInterfaceNoLock();

    // Remove from simulation and destroy
    bodyInterface.RemoveBody(bodyID);
    bodyInterface.DestroyBody(bodyID);

    // Clean up maps
    m_bodyToEntity.erase(bodyID);
    m_entityToBody.erase(it);
}

void PhysicsWorld::DestroyAllBodies()
{
    if (!m_physicsSystem) return;
    JPH::BodyInterface& bodyInterface = m_physicsSystem->GetBodyInterface();

    std::vector<JPH::BodyID> toRemove;
    for (auto& [entity, bodyID] : m_entityToBody)
    {
        toRemove.push_back(bodyID);
    }

    if (!toRemove.empty()) {
        bodyInterface.RemoveBodies(toRemove.data(), static_cast<int>(toRemove.size()));
        bodyInterface.DestroyBodies(toRemove.data(), static_cast<int>(toRemove.size()));
    }

    m_entityToBody.clear();
    m_bodyToEntity.clear();

    for (auto& pair : m_characters) {
        delete pair.second;
    }
    m_characters.clear();
}

glm::vec3 PhysicsWorld::GetCenterOfMassOffset(entt::entity entity) const
{
    auto it = m_entityToBody.find(entity);
    if (it == m_entityToBody.end()) return glm::vec3(0.0f);

    const JPH::BodyLockInterfaceNoLock& lockInterface = m_physicsSystem->GetBodyLockInterfaceNoLock();
    JPH::BodyLockRead bodyLock(lockInterface, it->second);

    if (bodyLock.Succeeded()) {
        const JPH::Body& body = bodyLock.GetBody();
        JPH::Vec3 com = body.GetShape()->GetCenterOfMass();
        return glm::vec3(com.GetX(), com.GetY(), com.GetZ());
    }
    return glm::vec3(0.0f);
}

// ------------------------------------------------------------
// 10. Queries / setters stubs
// ------------------------------------------------------------
bool PhysicsWorld::HasBody(entt::entity entity) const
{
    auto it = m_entityToBody.find(entity);
    return it != m_entityToBody.end();
}

glm::vec3 PhysicsWorld::GetBodyPosition(entt::entity entity) const
{
    auto it = m_entityToBody.find(entity);
    if (it == m_entityToBody.end())
        return glm::vec3(0.0f);

    JPH::BodyID bodyID = it->second;

    // Get a REFERENCE, not a copy
    const JPH::BodyLockInterfaceNoLock& lockInterface = m_physicsSystem->GetBodyLockInterfaceNoLock();

    JPH::BodyLockRead bodyLock(lockInterface, bodyID);
    if (bodyLock.Succeeded())
    {
        const JPH::Body& body = bodyLock.GetBody();
        JPH::Vec3 p = body.GetPosition();
        return glm::vec3(p.GetX(), p.GetY(), p.GetZ());
    }

    return glm::vec3(0.0f);
}

glm::quat PhysicsWorld::GetBodyRotation(entt::entity entity) const
{
    auto it = m_entityToBody.find(entity);
    if (it == m_entityToBody.end())
        return glm::quat(1, 0, 0, 0);

    JPH::BodyID bodyID = it->second;

    const JPH::BodyLockInterfaceNoLock& lockInterface = m_physicsSystem->GetBodyLockInterfaceNoLock();

    JPH::BodyLockRead bodyLock(lockInterface, bodyID);
    if (bodyLock.Succeeded())
    {
        const JPH::Body& body = bodyLock.GetBody();
        JPH::Quat r = body.GetRotation();
        // Jolt quat = x,y,z,w. glm quat = w,x,y,z.
        return glm::quat(r.GetW(), r.GetX(), r.GetY(), r.GetZ());
    }

    return glm::quat(1, 0, 0, 0);
}

glm::vec3 PhysicsWorld::GetBodyVelocity(entt::entity entity) const
{
    auto it = m_entityToBody.find(entity);
    if (it == m_entityToBody.end())
        return glm::vec3(0.0f);

    JPH::BodyID bodyID = it->second;

    const JPH::BodyLockInterfaceNoLock& lockInterface = m_physicsSystem->GetBodyLockInterfaceNoLock();

    JPH::BodyLockRead bodyLock(lockInterface, bodyID);
    if (bodyLock.Succeeded())
    {
        const JPH::Body& body = bodyLock.GetBody();
        JPH::Vec3 v = body.GetLinearVelocity();
        return glm::vec3(v.GetX(), v.GetY(), v.GetZ());
    }

    return glm::vec3(0.0f);
}

void PhysicsWorld::SetLinearVelocity(entt::entity entity, const glm::vec3& velocity)
{
    auto it = m_entityToBody.find(entity);
    if (it == m_entityToBody.end())
        return;

    JPH::BodyID bodyID = it->second;

    JPH::BodyInterface& bodyInterface = m_physicsSystem->GetBodyInterfaceNoLock();
    bodyInterface.SetLinearVelocity(
        bodyID,
        JPH::Vec3(velocity.x, velocity.y, velocity.z)
    );
}

void PhysicsWorld::AddForce(entt::entity entity, const glm::vec3& force)
{
    auto it = m_entityToBody.find(entity);
    if (it == m_entityToBody.end())
        return;

    JPH::BodyID bodyID = it->second;

    JPH::BodyInterface& bodyInterface = m_physicsSystem->GetBodyInterfaceNoLock();
    bodyInterface.AddForce(
        bodyID,
        JPH::Vec3(force.x, force.y, force.z)
    );
}

static inline glm::quat EulerDegToQuat(const glm::vec3& deg) {
    return glm::quat(glm::radians(deg));
}

bool PhysicsWorld::RaycastAgainstColliders(const glm::vec3& origin, const glm::vec3& direction, float maxDistance, RaycastHit& outHit)
{
    auto view = registry.view<TransformComponent, ColliderComponent>();

    bool foundHit = false;
    float closestT = maxDistance;

    for (auto [entity, transform, collider] : view.each())
    {
        //Construct model matrix
        glm::mat4 model = glm::mat4(1.f);
        model = glm::translate(model, transform.position);

        //Apply rotation
        model = glm::rotate(model, glm::radians(transform.rotation.y), glm::vec3(0, 1, 0));
        model = glm::rotate(model, glm::radians(transform.rotation.x), glm::vec3(1, 0, 0));
        model = glm::rotate(model, glm::radians(transform.rotation.z), glm::vec3(0, 0, 1));

        model = glm::scale(model, transform.scale);

        glm::mat4 invModel = glm::inverse(model);

        //Transform ray to local space
        //Position affects origin (w = 1)
        glm::vec4 localOriginV4 = invModel * glm::vec4(origin, 1.f);
        glm::vec3 localOrigin = glm::vec3(localOriginV4);

        //Direction affects vector (w = 0), to normalize after scaling
        glm::vec4 localDirV4 = invModel * glm::vec4(direction, 0.f);
        glm::vec3 localDir = glm::normalize(localDirV4);

        //Slab Test against local aabb
        glm::vec3 aabbMin = collider.minBounds;
        glm::vec3 aabbMax = collider.maxBounds;

        if (aabbMin == aabbMax) continue;

        float t1 = (aabbMin.x - localOrigin.x) / localDir.x;
        float t2 = (aabbMax.x - localOrigin.x) / localDir.x;
        float t3 = (aabbMin.y - localOrigin.y) / localDir.y;
        float t4 = (aabbMax.y - localOrigin.y) / localDir.y;
        float t5 = (aabbMin.z - localOrigin.z) / localDir.z;
        float t6 = (aabbMax.z - localOrigin.z) / localDir.z;

        float tmin = std::max(std::max(std::min(t1, t2), std::min(t3, t4)), std::min(t5, t6));
        float tmax = std::min(std::min(std::max(t1, t2), std::max(t3, t4)), std::max(t5, t6));

        //tmax >= 0 means intersection is not behind us
        //tmin <= tmax means ray pass through box
        if (tmax >= 0 && tmin <= tmax)
        {
            //Convert local dist back to world dist
            glm::vec3 localHitPos = localOrigin + (localDir * tmin);

            //transform to world space
            glm::vec4 worldHitPos = model * glm::vec4(localHitPos, 1.f);
            float dist = glm::distance(origin, glm::vec3(worldHitPos));

            if (dist < closestT && dist > 0.f)
            {
                closestT = dist;
                outHit.hasHit = true;
                outHit.entity = entity;
                outHit.distance = closestT;
                outHit.position = glm::vec3(worldHitPos);
                foundHit = true;
            }
        }
    }

    auto viewTrig = registry.view<TransformComponent, TriggerBoxComponent>();
    for (auto [entity, transform, trigger] : viewTrig.each())
    {
        if (!trigger.enabled) continue;

        glm::mat4 model = glm::mat4(1.f);
        model = glm::translate(model, transform.position);
        model = glm::rotate(model, glm::radians(transform.rotation.y), glm::vec3(0, 1, 0));
        model = glm::rotate(model, glm::radians(transform.rotation.x), glm::vec3(1, 0, 0));
        model = glm::rotate(model, glm::radians(transform.rotation.z), glm::vec3(0, 0, 1));
        model = glm::scale(model, transform.scale);

        glm::mat4 triggerLocal = glm::translate(glm::mat4(1.0f), trigger.localPosition);
        triggerLocal *= glm::mat4_cast(EulerDegToQuat(trigger.localEulerDeg));

        glm::mat4 totalTransform = model * triggerLocal;
        glm::mat4 invTotal = glm::inverse(totalTransform);

        glm::vec3 localOrigin = glm::vec3(invTotal * glm::vec4(origin, 1.0f));
        glm::vec3 localDir = glm::normalize(glm::vec3(invTotal * glm::vec4(direction, 0.0f)));
        glm::vec3 minB(0.0f), maxB(0.0f);

        switch (trigger.type)
        {
        case TriggerShapeType::Box:
            minB = -trigger.halfExtents;
            maxB = trigger.halfExtents;
            break;
        case TriggerShapeType::Sphere:
            minB = glm::vec3(-trigger.radius);
            maxB = glm::vec3(trigger.radius);
            break;
        case TriggerShapeType::Capsule:
            //Approximate capsule as a Box for selection click
            //Width/Depth = Radius, Height = HalfHeight + Radius
            float h = trigger.capHalfHeight + trigger.capRadius;
            float r = trigger.capRadius;
            minB = glm::vec3(-r, -h, -r);
            maxB = glm::vec3(r, h, r);
            break;
        }

        float t1 = (minB.x - localOrigin.x) / localDir.x;
        float t2 = (maxB.x - localOrigin.x) / localDir.x;
        float t3 = (minB.y - localOrigin.y) / localDir.y;
        float t4 = (maxB.y - localOrigin.y) / localDir.y;
        float t5 = (minB.z - localOrigin.z) / localDir.z;
        float t6 = (maxB.z - localOrigin.z) / localDir.z;

        float tmin = std::max(std::max(std::min(t1, t2), std::min(t3, t4)), std::min(t5, t6));
        float tmax = std::min(std::min(std::max(t1, t2), std::max(t3, t4)), std::max(t5, t6));
        if (tmax >= 0 && tmin <= tmax)
        {
            // Convert back to World Space for distance check
            glm::vec3 localHitPos = localOrigin + (localDir * tmin);
            glm::vec4 worldHitPos = totalTransform * glm::vec4(localHitPos, 1.0f);
            float dist = glm::distance(origin, glm::vec3(worldHitPos));

            if (dist < closestT && dist > 0.0f)
            {
                closestT = dist;
                outHit.hasHit = true;
                outHit.entity = entity;
                outHit.distance = closestT;
                outHit.position = glm::vec3(worldHitPos);
                foundHit = true;
            }
        }
    }

    return foundHit;
}

bool PhysicsWorld::Raycast(const glm::vec3& origin, const glm::vec3& direction, float maxDistance, RaycastHit& outHit)
{
    if (!m_physicsSystem) return false;

    struct RaycastFilterAll : public JPH::BroadPhaseLayerFilter, public JPH::ObjectLayerFilter, public JPH::BodyFilter
    {
        // BroadPhase: Hit Static, Dynamic, and Editor layers
        virtual bool ShouldCollide(JPH::BroadPhaseLayer /*inLayer*/) const override { return true; }

        // Object Layer: Hit Static, Dynamic, and Editor Ghost layers
        virtual bool ShouldCollide(JPH::ObjectLayer /*inLayer*/) const override { return true; }

        // Body: Hit any body ID
        virtual bool ShouldCollide(const JPH::BodyID& /*inBodyID*/) const override { return true; }

        // Body Locked: Allow hitting bodies even if locked (standard check)
        virtual bool ShouldCollideLocked(const JPH::Body& /*inBody*/) const override { return true; }
    };

    // 1. Setup Jolt Ray
    // Jolt uses RVec3 (double precision) for positions, Vec3 for direction * distance
    JPH::RVec3 jOrigin(origin.x, origin.y, origin.z);
    JPH::Vec3 jDirection(direction.x * maxDistance, direction.y * maxDistance, direction.z * maxDistance);

    JPH::RRayCast ray(jOrigin, jDirection);
    JPH::RayCastResult hit;
    RaycastFilterAll filter;
    const JPH::NarrowPhaseQuery& query = m_physicsSystem->GetNarrowPhaseQueryNoLock();

    bool success = query.CastRay(
        ray,
        hit,
        filter, // Hit all layers
        filter,     // Hit all objects
        filter            // Hit all bodies
    );

    if (success)
    {
        outHit.hasHit = true;
        outHit.distance = hit.mFraction * maxDistance;

        // Calculate world position
        JPH::RVec3 hitPos = ray.GetPointOnRay(hit.mFraction);
        outHit.position = glm::vec3(hitPos.GetX(), hitPos.GetY(), hitPos.GetZ());

        JPH::BodyLockRead lock(m_physicsSystem->GetBodyLockInterface(), hit.mBodyID);
        if (lock.Succeeded())
        {
            // 4. Retrieve Entity from User Data
            const JPH::Body& body = lock.GetBody();
            uint64_t userData = body.GetUserData();
            outHit.entity = static_cast<entt::entity>(static_cast<uint32_t>(userData));
        }
        return true;
    }

    outHit.hasHit = false;
    return false;
}

bool PhysicsWorld::AreEntitiesColliding(entt::entity entityA, entt::entity entityB)
{
    if (!registry.valid(entityA) || !registry.valid(entityB)) return false;
    JPH::AABox boundsA, boundsB;

    auto GetEntityBounds = [&](entt::entity e, JPH::AABox& outBounds) -> bool
    {
        //Jolt check first
        if (m_entityToBody.find(e) != m_entityToBody.end())
        {
            JPH::BodyID bodyID = m_entityToBody[e];
            JPH::BodyLockRead lock(m_physicsSystem->GetBodyLockInterface(), bodyID);
            if (lock.Succeeded())
            {
                const JPH::Body& body = lock.GetBody();
                outBounds = body.GetWorldSpaceBounds();
                return true;
            }
        }
        //if fail, manual check
        if (registry.all_of<TransformComponent, ColliderComponent>(e))
        {
            const auto& tf = registry.get<TransformComponent>(e);
            const auto& col = registry.get<ColliderComponent>(e);

            glm::vec3 localSize = col.maxBounds - col.minBounds;
            glm::vec3 worldSize = localSize * tf.scale;
            glm::vec3 localCenter = (col.minBounds + col.maxBounds) * 0.5f;
            glm::vec3 worldCenter = tf.position + (localCenter * tf.scale);

            JPH::Vec3 jMin(
                worldCenter.x - std::abs(worldSize.x) * 0.5f,
                worldCenter.y - std::abs(worldSize.y) * 0.5f,
                worldCenter.z - std::abs(worldSize.z) * 0.5f
            );
            JPH::Vec3 jMax(
                worldCenter.x + std::abs(worldSize.x) * 0.5f,
                worldCenter.y + std::abs(worldSize.y) * 0.5f,
                worldCenter.z + std::abs(worldSize.z) * 0.5f
            );

            outBounds = JPH::AABox(jMin, jMax);
            return true;
        }

        return false;
    };

    if (!GetEntityBounds(entityA, boundsA)) return false;
    if (!GetEntityBounds(entityB, boundsB)) return false;

    return boundsA.Overlaps(boundsB);
}

void PhysicsWorld::ApplyImpulse(entt::entity entity, float x, float y, float z)
{
    if (m_entityToBody.find(entity) == m_entityToBody.end()) return; //Body doesnt exist

    JPH::BodyID bodyID = m_entityToBody[entity];
    JPH::BodyInterface& bodyInterface = m_physicsSystem->GetBodyInterface();

    bodyInterface.ActivateBody(bodyID); //Have to wake body to apply physics

    bodyInterface.AddImpulse(bodyID, JPH::Vec3(x, y, z)); //Apply impulse force
}

void PhysicsWorld::ExplodeGroup(const std::vector<entt::entity>& parts, float strength)
{
    JPH::BodyInterface& bodyInterface = m_physicsSystem->GetBodyInterface();

    for (entt::entity part : parts)
    {
        //Locate entity physics body
        if (m_entityToBody.find(part) == m_entityToBody.end()) continue;
        JPH::BodyID bodyID = m_entityToBody[part];

        //Generate Random Direction
        float rx = ((float)rand() / RAND_MAX) * 2.f - 1.f; //-1 to 1 (left - right)
        float ry = ((float)rand() / RAND_MAX);             //0 to 2 (only upwards)
        float rz = ((float)rand() / RAND_MAX) * 2.f - 1.f; //-1 ot 1 (forwards - backwards)

        //Normalize
        float length = std::sqrt(rx * rx + ry * ry + rz * rz);
        if (length > 0) { rx /= length; ry /= length; rz /= length; }

        bodyInterface.ActivateBody(bodyID);
        bodyInterface.AddImpulse(bodyID, JPH::Vec3(rx, ry, rz) * strength);
    }
}

void PhysicsWorld::ApplyRadialExplosion(const std::vector<entt::entity>& parts, float centerX, float centerY, float centerZ, float strength)
{
    JPH::BodyInterface& bodyInterface = m_physicsSystem->GetBodyInterface();
    JPH::Vec3 center(centerX, centerY, centerZ);

    for (entt::entity part : parts)
    {
        if (m_entityToBody.find(part) == m_entityToBody.end()) continue;

        JPH::BodyID bodyID = m_entityToBody[part];

        //Get position and rotation of part
        JPH::Vec3 partPos = bodyInterface.GetPosition(bodyID);
        JPH::Vec3 dir = partPos - center;

        //Normalize
        if (dir.Length() < 0.001f) dir = JPH::Vec3(0, 1, 0);
        else dir = dir.Normalized();

        bodyInterface.ActivateBody(bodyID);
        bodyInterface.AddImpulse(bodyID, dir * strength);
    }
}

void PhysicsWorld::CreateCharacter(entt::entity entity, const glm::vec3& pos, float height, float radius, float mass, float maxSlopeAngle, float jumpStrength)
{
    if (HasCharacter(entity)) return;

    float cylinderHalfHeight = (height * 0.5f) - radius;
    if (cylinderHalfHeight < 0.01f) cylinderHalfHeight = 0.01f;

    JPH::Ref<JPH::CapsuleShapeSettings> shapeSettings = new JPH::CapsuleShapeSettings(cylinderHalfHeight, radius);
    JPH::Ref<JPH::CharacterVirtualSettings> charSettings = new JPH::CharacterVirtualSettings();
    charSettings->mShape = shapeSettings->Create().Get();
    charSettings->mMass = mass;
    charSettings->mMaxSlopeAngle = glm::radians(maxSlopeAngle);
    charSettings->mMaxStrength = jumpStrength;
    charSettings->mUp = JPH::Vec3(0, 1, 0);
    charSettings->mCharacterPadding = 0.02f;
    charSettings->mPenetrationRecoverySpeed = 1.f;
    charSettings->mPredictiveContactDistance = 0.1f;

    JPH::CharacterVirtual* character = new JPH::CharacterVirtual(charSettings, JPH::RVec3(pos.x, pos.y, pos.z), JPH::Quat::sIdentity(), m_physicsSystem);
    m_characters[entity] = character;
}

void PhysicsWorld::DestroyCharacter(entt::entity entity)
{
    auto it = m_characters.find(entity);
    if (it != m_characters.end())
    {
        delete it->second;
        m_characters.erase(it);
    }
}

glm::vec3 PhysicsWorld::UpdateCharacter(entt::entity entity, const glm::vec3& inputVelocity, float dt, float gravityFactor, bool& outIsGrounded, glm::vec3& outGroundNormal, glm::vec3& outVelocity)
{
    auto it = m_characters.find(entity);
    if (it == m_characters.end()) return glm::vec3(0.0f);
    JPH::CharacterVirtual* character = it->second;
    //Apply gravity and velocity
    JPH::Vec3 globalGravity = m_physicsSystem->GetGravity();
    JPH::Vec3 currentVel = character->GetLinearVelocity();
    JPH::Vec3 targetVel(inputVelocity.x, inputVelocity.y, inputVelocity.z);
    
    bool wasGrounded = (character->GetGroundState() == JPH::CharacterVirtual::EGroundState::OnGround);
    if (!wasGrounded)
    {
        JPH::Vec3 effectiveGravity = globalGravity * gravityFactor;
        targetVel.SetY(currentVel.GetY() + (effectiveGravity.GetY() * dt));
    }
    else if (targetVel.GetY() < 0.1f)
    {
        targetVel.SetY(std::max((float)targetVel.GetY(), -2.f));
    }

    character->SetLinearVelocity(targetVel);

    JPH::DefaultBroadPhaseLayerFilter bpFilter(*m_objectVsBroadPhaseLayerFilter, Layers::DYNAMIC);
    JPH::DefaultObjectLayerFilter layerFilter(*m_objectLayerPairFilter, Layers::DYNAMIC);
    JPH::BodyFilter bodyFilter;
    JPH::ShapeFilter shapeFilter;

    character->Update(dt, globalGravity, bpFilter, layerFilter, bodyFilter, shapeFilter, *m_tempAllocator);

    JPH::RVec3 pos = character->GetPosition();
    JPH::Vec3 gn = character->GetGroundNormal();
    JPH::Vec3 vel = character->GetLinearVelocity();

    outIsGrounded = (character->GetGroundState() == JPH::CharacterVirtual::EGroundState::OnGround);
    outGroundNormal = glm::vec3(gn.GetX(), gn.GetY(), gn.GetZ());
    outVelocity = glm::vec3(vel.GetX(), vel.GetY(), vel.GetZ());

    return glm::vec3(pos.GetX(), pos.GetY(), pos.GetZ());
}

bool PhysicsWorld::HasCharacter(entt::entity entity) const
{
    return m_characters.find(entity) != m_characters.end();
}

void PhysicsWorld::SetCharacterSettings(entt::entity entity, float mass, float maxSlopeAngle)
{
    auto it = m_characters.find(entity);
    if (it != m_characters.end())
    {
        it->second->SetMass(mass);
        it->second->SetMaxSlopeAngle(glm::radians(maxSlopeAngle));
    }
}
