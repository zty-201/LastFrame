#pragma once
#include <Jolt/Jolt.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <unordered_map>
#include <entt/entity/entity.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <memory>
#include <Jolt/Physics/Collision/ContactListener.h>
#include <limits>

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
    static constexpr JPH::ObjectLayer EDITOR_GHOST = 2;
    static constexpr JPH::ObjectLayer NUM_LAYERS = 3;
}

static inline JPH::ObjectLayer GetObjectLayerForIsStatic(bool isStatic)
{
    return isStatic ? Layers::STATIC : Layers::DYNAMIC;
}

namespace BroadPhaseLayers
{
    static constexpr JPH::BroadPhaseLayer STATIC_LAYER = JPH::BroadPhaseLayer(0);
    static constexpr JPH::BroadPhaseLayer DYNAMIC_LAYER = JPH::BroadPhaseLayer(1);
    static constexpr JPH::BroadPhaseLayer EDITOR_LAYER(2);
    static constexpr uint32_t NUM_LAYERS = 3;
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
        mObjectToBroadPhase[Layers::EDITOR_GHOST] = BroadPhaseLayers::EDITOR_LAYER;
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
        case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::EDITOR_LAYER:   return "EDITOR";
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
        case Layers::EDITOR_GHOST:
            return false;
        default:
            return false;
        }
    }
};

class ObjectLayerPairFilterImpl : public JPH::ObjectLayerPairFilter
{
public:
    virtual bool ShouldCollide(JPH::ObjectLayer inLayer1,
        JPH::ObjectLayer inLayer2) const override
    {
        if (inLayer1 == Layers::EDITOR_GHOST || inLayer2 == Layers::EDITOR_GHOST)
            return false;

        // Allow dynamic<->dynamic and dynamic<->static.
        // We can choose to ignore static<->static (no need for 2 floors to collide).
        if (inLayer1 == Layers::DYNAMIC && inLayer2 == Layers::DYNAMIC) return true;
        if (inLayer1 == Layers::DYNAMIC && inLayer2 == Layers::STATIC)  return true;
        if (inLayer1 == Layers::STATIC && inLayer2 == Layers::DYNAMIC) return true;
        return false;
    }
};

struct RaycastHit
{
    entt::entity entity = entt::null;
    glm::vec3 position{ 0.0f };
    float distance = 0.0f;
    bool hasHit = false;
};

class PhysicsWorld
{
public:
    static PhysicsWorld& Get(); // Singleton access

    void Initialize();
    void Shutdown();

    void StepSimulation(float dt);

    // Body management
    void CreatePhysicsBody(
        entt::entity entity,
        const glm::vec3& pos,
        const glm::vec3& rot,
        const glm::vec3& scale,
        const struct ColliderComponent& collider,
        const struct PhysicsMaterialComponent& material,
        const float gravityFactor
    );
    /*void CreateBoxBody(entt::entity entity,
        const glm::vec3& position,
        const glm::vec3& size,
        const float density,
        const float gravityFactor,
        bool isStatic,
        bool isTrigger = false);

    void CreateSphereBody(entt::entity entity,
        const glm::vec3& position,
        float radius,
        const float density,
        const float gravityFactor,
        bool isStatic,
        bool isTrigger = false);

    void CreateCapsuleBody(entt::entity entity,
        const glm::vec3& position,
        float height,
        float radius,
        const float density,
        const float gravityFactor,
        bool isStatic,
        bool isTrigger = false);*/

    void RemoveBody(entt::entity entity);
    void DestroyAllBodies();

    glm::vec3 GetCenterOfMassOffset(entt::entity entity) const;

    void SetGravityFactor(entt::entity entity, float factor);

    void SetPosition(entt::entity e, const glm::vec3& p, bool activate = true);
    void SetRotation(entt::entity e, const glm::quat& q, bool activate = true);
    void SetPositionRotation(entt::entity e, const glm::vec3& p, const glm::quat& q, bool activate = true);
    void ActivateBody(entt::entity e);

    // Body state access
    bool HasBody(entt::entity entity) const;
    glm::vec3 GetBodyPosition(entt::entity entity) const;
    glm::quat GetBodyRotation(entt::entity entity) const;
    glm::vec3 GetBodyVelocity(entt::entity entity) const;

    void SetLinearVelocity(entt::entity entity, const glm::vec3& velocity);
    void AddForce(entt::entity entity, const glm::vec3& force);

#if defined(PLATFORM_WINDOWS) || defined(DEBUG)
    bool RaycastAgainstColliders(const glm::vec3& origin, const glm::vec3& direction, float maxDistance, RaycastHit& outHit);
#endif
    bool Raycast(const glm::vec3& origin, const glm::vec3& direction, float maxDistance, RaycastHit& outHit);

    /*************************************************
    * This function applies an impulse force on a singular entity.
    * This is method 1 of 3 to explode items.
    * More control over where you want parts to fly, but you have more code
    * to write; you have to calculate direction yourself.
    *************************************************/
    void ApplyImpulse(entt::entity entity, float x, float y, float z);

    /*************************************************
    * This function applies impulse forces on all parts of the item.
    * This is method 2 of 3 to explode items.
    * Less control over where you wants parts to fly, but you will have
    * less code to write; you just need to pass the list of parts and how
    * strong you want the explosion to be.
    *************************************************/
    void ExplodeGroup(const std::vector<entt::entity>& parts, float strength);

    /*************************************************
    * This function applies force in the most "natrual" way as
    * an explosion would take place; outwards. Just pass in list
    * of parts and the origin of explosion. Strength of explosion as well.
    *************************************************/
    void ApplyRadialExplosion(const std::vector<entt::entity>& parts, float centerX, float centerY, float centerZ, float strength);

private:
    PhysicsWorld() = default;
    ~PhysicsWorld() = default;

    JPH::PhysicsSystem* m_physicsSystem = nullptr;
    JPH::JobSystemThreadPool* m_jobSystem = nullptr;

    std::unique_ptr<BroadPhaseLayerInterfaceImpl> m_broadPhaseLayerInterface;
    std::unique_ptr<ObjectVsBroadPhaseLayerFilterImpl> m_objectVsBroadPhaseLayerFilter;
    std::unique_ptr<ObjectLayerPairFilterImpl> m_objectLayerPairFilter;

    std::unique_ptr<JPH::TempAllocatorImpl> m_tempAllocator;

    std::unordered_map<entt::entity, JPH::BodyID> m_entityToBody;
    std::unordered_map<JPH::BodyID, entt::entity> m_bodyToEntity;

    class ContactListenerImpl final : public JPH::ContactListener {
    public:
        ContactListenerImpl() = default;

        // Non-sensor contacts
        void OnContactAdded(const JPH::Body& b1,
            const JPH::Body& b2,
            const JPH::ContactManifold&,
            JPH::ContactSettings&) override
        {
            const uint32_t a = (uint32_t)b1.GetUserData();
            const uint32_t b = (uint32_t)b2.GetUserData();
            //LOGI("[Physics] Collision Enter: %u <-> %u", a, b);
            JPH_UNUSED(a);
            JPH_UNUSED(b);
        }

        void OnContactRemoved(const JPH::SubShapeIDPair& p) override
        {
            // We only have BodyIDs here; avoid BodyInterface to keep things lock-safe.
            // You can still log something useful:
            uint32_t id1 = p.GetBody1ID().GetIndexAndSequenceNumber();
            uint32_t id2 = p.GetBody2ID().GetIndexAndSequenceNumber();
            //LOGI("[Physics] Collision Exit (BodyID): %u <-> %u", id1, id2);
            JPH_UNUSED(id1);
            JPH_UNUSED(id2);
        }

        // Optional: sensor hooks, keep them simple & not using BodyInterface
        void OnSensorContactAdded(const JPH::BodyID& sensor,
            const JPH::BodyID& other,
            const JPH::SubShapeIDPair&)
        {
            uint32_t s = sensor.GetIndexAndSequenceNumber();
            uint32_t o = other.GetIndexAndSequenceNumber();
            //LOGI("[Physics] Trigger Enter (BodyID): %u <-> %u", s, o);
            JPH_UNUSED(s);
            JPH_UNUSED(o);
        }

        void OnSensorContactRemoved(const JPH::BodyID& sensor,
            const JPH::BodyID& other,
            const JPH::SubShapeIDPair&)
        {
            uint32_t s = sensor.GetIndexAndSequenceNumber();
            uint32_t o = other.GetIndexAndSequenceNumber();
            //LOGI("[Physics] Trigger Exit (BodyID): %u <-> %u", s, o);
            JPH_UNUSED(s);
            JPH_UNUSED(o);
        }
    };

    // Listener instance (note: declared AFTER m_bodyToEntity so we can pass its address)
    ContactListenerImpl m_contactListener;

    float m_accumulator = 0.0f;
    const float m_fixedDt = 1.0f / 60.0f;
};