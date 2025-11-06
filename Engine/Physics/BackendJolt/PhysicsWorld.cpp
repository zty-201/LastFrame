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

using uint = unsigned int;

// ------------------------------------------------------------
// 1. Layer definitions
// ------------------------------------------------------------
//
// ObjectLayer is used to tag individual bodies.
// BroadPhaseLayer is used internally by broadphase structures.
// We’ll keep it super simple: STATIC vs DYNAMIC.

namespace Layers
{
    static constexpr JPH::ObjectLayer STATIC = 0;
    static constexpr JPH::ObjectLayer DYNAMIC = 1;
    static constexpr JPH::ObjectLayer NUM_LAYERS = 2;
}

static inline JPH::ObjectLayer GetObjectLayerForIsStatic(bool isStatic)
{
    return isStatic ? Layers::STATIC : Layers::DYNAMIC;
}

namespace BroadPhaseLayers
{
    static constexpr JPH::BroadPhaseLayer STATIC_LAYER = JPH::BroadPhaseLayer(0);
    static constexpr JPH::BroadPhaseLayer DYNAMIC_LAYER = JPH::BroadPhaseLayer(1);
    static constexpr uint32_t NUM_LAYERS = 2;
}

// ------------------------------------------------------------
// 2. BroadPhaseLayerInterface impl
//    Maps object layer -> broadphase layer
// ------------------------------------------------------------
class BroadPhaseLayerInterfaceImpl final : public JPH::BroadPhaseLayerInterface
{
public:
    BroadPhaseLayerInterfaceImpl()
    {
        // STATIC objects -> STATIC broadphase layer
        mObjectToBroadPhase[Layers::STATIC] = BroadPhaseLayers::STATIC_LAYER;
        // DYNAMIC objects -> DYNAMIC broadphase layer
        mObjectToBroadPhase[Layers::DYNAMIC] = BroadPhaseLayers::DYNAMIC_LAYER;
    }

    virtual uint GetNumBroadPhaseLayers() const override
    {
        return BroadPhaseLayers::NUM_LAYERS;
    }

    virtual JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override
    {
        assert(inLayer < Layers::NUM_LAYERS);
        return mObjectToBroadPhase[inLayer];
    }

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
    // Optional: debug names
    virtual const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const override
    {
        switch ((JPH::BroadPhaseLayer::Type)inLayer)
        {
        case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::STATIC_LAYER:   return "STATIC";
        case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::DYNAMIC_LAYER:  return "DYNAMIC";
        default:                                                           return "UNKNOWN";
        }
    }
#endif

private:
    JPH::BroadPhaseLayer mObjectToBroadPhase[Layers::NUM_LAYERS];
};

// ------------------------------------------------------------
// 3. ObjectVsBroadPhaseLayerFilter
//    Says which object layers should collide with which broadphase layer
// ------------------------------------------------------------
class ObjectVsBroadPhaseLayerFilterImpl : public JPH::ObjectVsBroadPhaseLayerFilter
{
public:
    virtual bool ShouldCollide(JPH::ObjectLayer inLayer1,
        JPH::BroadPhaseLayer inLayer2) const override
    {
        switch (inLayer1)
        {
        case Layers::STATIC:
            // static doesn't collide with static? up to us.
            // Usually: static vs dynamic only
            return inLayer2 == BroadPhaseLayers::DYNAMIC_LAYER;

        case Layers::DYNAMIC:
            // dynamic collides with both
            return (inLayer2 == BroadPhaseLayers::STATIC_LAYER) ||
                (inLayer2 == BroadPhaseLayers::DYNAMIC_LAYER);

        default:
            return false;
        }
    }
};

// ------------------------------------------------------------
// 4. ObjectLayerPairFilter
//    Says which two object layers can collide with each other
// ------------------------------------------------------------
class ObjectLayerPairFilterImpl : public JPH::ObjectLayerPairFilter
{
public:
    virtual bool ShouldCollide(JPH::ObjectLayer inLayer1,
        JPH::ObjectLayer inLayer2) const override
    {
        // Allow dynamic<->dynamic and dynamic<->static.
        // We can choose to ignore static<->static (no need for 2 floors to collide).
        if (inLayer1 == Layers::DYNAMIC && inLayer2 == Layers::DYNAMIC) return true;
        if (inLayer1 == Layers::DYNAMIC && inLayer2 == Layers::STATIC)  return true;
        if (inLayer1 == Layers::STATIC && inLayer2 == Layers::DYNAMIC) return true;
        return false;
    }
};

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

    // 6.3 Create filter objects
    // VERY IMPORTANT:
    // These objects must live as long as PhysicsSystem lives.
    // We'll make them static so their lifetime is entire program.
    static BroadPhaseLayerInterfaceImpl broadPhaseLayerInterface;
    static ObjectVsBroadPhaseLayerFilterImpl objectVsBroadPhaseLayerFilter;
    static ObjectLayerPairFilterImpl objectLayerPairFilter;

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
        broadPhaseLayerInterface,
        objectVsBroadPhaseLayerFilter,
        objectLayerPairFilter
    );

    // 6.6 Set gravity (Y down)
    m_physicsSystem->SetGravity(JPH::Vec3(0.0f, -9.81f, 0.0f));

    // Reset step accumulator
    m_accumulator = 0.0f;

    LOGI("[PhysicsWorld] Initialized Jolt Physics");
}

// ------------------------------------------------------------
// 7. Shutdown()
// ------------------------------------------------------------
void PhysicsWorld::Shutdown()
{
    // Destroy physics system
    if (m_physicsSystem)
    {
        delete m_physicsSystem;
        m_physicsSystem = nullptr;
    }

    // Destroy job system
    if (m_jobSystem)
    {
        delete m_jobSystem;
        m_jobSystem = nullptr;
    }

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
            JPH::TempAllocatorImpl temp_allocator(10 * 1024);

            // Advance the simulation by one fixed tick.
            // collisionSteps = 1 is fine for now.
            // We ignore the return value (EPhysicsUpdateError).
            (void)m_physicsSystem->Update(
                m_fixedDt,
                1,                  // collision steps
                &temp_allocator,    // temp allocator for this step
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

void PhysicsWorld::CreateBoxBody(entt::entity entity,
    const glm::vec3& position,
    const glm::vec3& size,
    bool isStatic)
{
    // Don't recreate if it already exists
    if (HasBody(entity))
        return;

    // Half extents for Jolt box shape.
    // Our ColliderComponent stores half extents, but the BuildSystem may pass full size
    // or half size depending on how we wire it. For safety we'll treat `size` here
    // as FULL size and divide by 2.0f. You can adjust later if needed.
    JPH::Vec3 halfExtent(
        0.5f * size.x,
        0.5f * size.y,
        0.5f * size.z
    );

    JPH::Ref<JPH::BoxShape> shape = new JPH::BoxShape(halfExtent);

    // start transform
    JPH::Vec3 jPos(position.x, position.y, position.z);
    JPH::Quat jRot = JPH::Quat::sIdentity();

    JPH::EMotionType motionType = isStatic
        ? JPH::EMotionType::Static
        : JPH::EMotionType::Dynamic;

    // Create body settings
    JPH::BodyCreationSettings bodySettings(
        shape,
        jPos,
        jRot,
        motionType,
        GetObjectLayerForIsStatic(isStatic)
    );

    // Create body
    JPH::BodyInterface& bodyInterface = m_physicsSystem->GetBodyInterfaceNoLock();
    JPH::Body* body = bodyInterface.CreateBody(bodySettings);

    if (!body)
        return; // failed to create (out of capacity etc.)

    // Add to physics world (activate if dynamic so it simulates immediately)
    bodyInterface.AddBody(body->GetID(),
        isStatic ? JPH::EActivation::DontActivate : JPH::EActivation::Activate);

    // Track mapping
    m_entityToBody[entity] = body->GetID();
    m_bodyToEntity[body->GetID()] = entity;
}

void PhysicsWorld::CreateSphereBody(entt::entity entity,
    const glm::vec3& position,
    float radius,
    bool isStatic)
{
    if (HasBody(entity))
        return;

    JPH::Ref<JPH::SphereShape> shape = new JPH::SphereShape(radius);

    JPH::Vec3 jPos(position.x, position.y, position.z);
    JPH::Quat jRot = JPH::Quat::sIdentity();

    JPH::EMotionType motionType = isStatic
        ? JPH::EMotionType::Static
        : JPH::EMotionType::Dynamic;

    JPH::BodyCreationSettings bodySettings(
        shape,
        jPos,
        jRot,
        motionType,
        GetObjectLayerForIsStatic(isStatic)
    );

    JPH::BodyInterface& bodyInterface = m_physicsSystem->GetBodyInterfaceNoLock();
    JPH::Body* body = bodyInterface.CreateBody(bodySettings);
    if (!body)
        return;

    bodyInterface.AddBody(body->GetID(),
        isStatic ? JPH::EActivation::DontActivate : JPH::EActivation::Activate);

    m_entityToBody[entity] = body->GetID();
    m_bodyToEntity[body->GetID()] = entity;
}

void PhysicsWorld::CreateCapsuleBody(entt::entity entity,
    const glm::vec3& position,
    float height,
    float radius,
    bool isStatic)
{
    if (HasBody(entity))
        return;

    // In Jolt, capsule height is the cylinder part, not including the hemispheres.
    // We'll assume caller already passes the "usable" capsule height.
    JPH::Ref<JPH::CapsuleShape> shape = new JPH::CapsuleShape(height * 0.5f, radius);

    JPH::Vec3 jPos(position.x, position.y, position.z);
    JPH::Quat jRot = JPH::Quat::sIdentity();

    JPH::EMotionType motionType = isStatic
        ? JPH::EMotionType::Static
        : JPH::EMotionType::Dynamic;

    JPH::BodyCreationSettings bodySettings(
        shape,
        jPos,
        jRot,
        motionType,
        GetObjectLayerForIsStatic(isStatic)
    );

    JPH::BodyInterface& bodyInterface = m_physicsSystem->GetBodyInterfaceNoLock();
    JPH::Body* body = bodyInterface.CreateBody(bodySettings);
    if (!body)
        return;

    bodyInterface.AddBody(body->GetID(),
        isStatic ? JPH::EActivation::DontActivate : JPH::EActivation::Activate);

    m_entityToBody[entity] = body->GetID();
    m_bodyToEntity[body->GetID()] = entity;
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