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

#include <cassert>
#include <iostream>

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
        entt::entity eA = (entt::entity)1;
        entt::entity eB = (entt::entity)2;

        glm::vec3 pos(0.0f, 0.0f, 0.0f);
        glm::vec3 size(1.0f, 1.0f, 1.0f);

        // A = dynamic, B = static
        CreateBoxBody(eA, pos, size, /*isStatic=*/false, /*isTrigger=*/false);
        CreateBoxBody(eB, pos, size, /*isStatic=*/true,  /*isTrigger=*/false);
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

// ------------------------------
// Box
// ------------------------------
void PhysicsWorld::CreateBoxBody(entt::entity entity,
    const glm::vec3& position,
    const glm::vec3& size,   // FULL extents from your ColliderComponent
    bool isStatic,
    bool isTrigger)
{
    if (HasBody(entity)) return;

    // Jolt wants half extents
    const JPH::Vec3 halfExtent(0.5f * size.x, 0.5f * size.y, 0.5f * size.z);
    JPH::Ref<JPH::BoxShape> shape = new JPH::BoxShape(halfExtent);

    const JPH::Vec3 jPos(position.x, position.y, position.z);
    const JPH::Quat jRot = JPH::Quat::sIdentity();

    const JPH::EMotionType motionType = isStatic
        ? JPH::EMotionType::Static
        : JPH::EMotionType::Dynamic;

    JPH::BodyCreationSettings bodySettings(
        shape, jPos, jRot, motionType, GetObjectLayerForIsStatic(isStatic)
    );
    bodySettings.mIsSensor = isTrigger; // <— make it a trigger/sensor if requested

    JPH::BodyInterface& bodyInterface = m_physicsSystem->GetBodyInterfaceNoLock();
    JPH::Body* body = bodyInterface.CreateBody(bodySettings);
    if (!body) return;

    body->SetUserData(static_cast<uint64_t>(static_cast<uint32_t>(entity)));

    bodyInterface.AddBody(
        body->GetID(),
        isStatic ? JPH::EActivation::DontActivate : JPH::EActivation::Activate
    );

    m_entityToBody[entity] = body->GetID();
    m_bodyToEntity[body->GetID()] = entity;
}

// ------------------------------
// Sphere
// ------------------------------
void PhysicsWorld::CreateSphereBody(entt::entity entity,
    const glm::vec3& position,
    float radius,
    bool isStatic,
    bool isTrigger)
{
    if (HasBody(entity)) return;

    JPH::Ref<JPH::SphereShape> shape = new JPH::SphereShape(radius);

    const JPH::Vec3 jPos(position.x, position.y, position.z);
    const JPH::Quat jRot = JPH::Quat::sIdentity();

    const JPH::EMotionType motionType = isStatic
        ? JPH::EMotionType::Static
        : JPH::EMotionType::Dynamic;

    JPH::BodyCreationSettings bodySettings(
        shape, jPos, jRot, motionType, GetObjectLayerForIsStatic(isStatic)
    );
    bodySettings.mIsSensor = isTrigger;

    JPH::BodyInterface& bodyInterface = m_physicsSystem->GetBodyInterfaceNoLock();
    JPH::Body* body = bodyInterface.CreateBody(bodySettings);
    if (!body) return;

    body->SetUserData(static_cast<uint64_t>(static_cast<uint32_t>(entity)));

    bodyInterface.AddBody(
        body->GetID(),
        isStatic ? JPH::EActivation::DontActivate : JPH::EActivation::Activate
    );

    m_entityToBody[entity] = body->GetID();
    m_bodyToEntity[body->GetID()] = entity;
}

// ------------------------------
// Capsule
// ------------------------------
void PhysicsWorld::CreateCapsuleBody(entt::entity entity,
    const glm::vec3& position,
    float height,   // your ColliderComponent stores full height
    float radius,
    bool isStatic,
    bool isTrigger)
{
    if (HasBody(entity)) return;

    // Jolt's CapsuleShape ctor takes half-height of the cylinder part
    JPH::Ref<JPH::CapsuleShape> shape = new JPH::CapsuleShape(0.5f * height, radius);

    const JPH::Vec3 jPos(position.x, position.y, position.z);
    const JPH::Quat jRot = JPH::Quat::sIdentity();

    const JPH::EMotionType motionType = isStatic
        ? JPH::EMotionType::Static
        : JPH::EMotionType::Dynamic;

    JPH::BodyCreationSettings bodySettings(
        shape, jPos, jRot, motionType, GetObjectLayerForIsStatic(isStatic)
    );
    bodySettings.mIsSensor = isTrigger;

    JPH::BodyInterface& bodyInterface = m_physicsSystem->GetBodyInterfaceNoLock();
    JPH::Body* body = bodyInterface.CreateBody(bodySettings);
    if (!body) return;

    body->SetUserData(static_cast<uint64_t>(static_cast<uint32_t>(entity)));

    bodyInterface.AddBody(
        body->GetID(),
        isStatic ? JPH::EActivation::DontActivate : JPH::EActivation::Activate
    );

    m_entityToBody[entity] = body->GetID();
    m_bodyToEntity[body->GetID()] = entity;
}

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