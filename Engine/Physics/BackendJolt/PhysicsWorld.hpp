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

class PhysicsWorld
{
public:
    static PhysicsWorld& Get(); // Singleton access

    void Initialize();
    void Shutdown();

    void StepSimulation(float dt);

    // Body management
    void CreateBoxBody(entt::entity entity, const glm::vec3& position, const glm::vec3& size, bool isStatic);
    void CreateSphereBody(entt::entity entity, const glm::vec3& position, float radius, bool isStatic);
    void CreateCapsuleBody(entt::entity entity, const glm::vec3& position, float height, float radius, bool isStatic);
    void RemoveBody(entt::entity entity);

    // Body state access
    bool HasBody(entt::entity entity) const;
    glm::vec3 GetBodyPosition(entt::entity entity) const;
    glm::quat GetBodyRotation(entt::entity entity) const;
    glm::vec3 GetBodyVelocity(entt::entity entity) const;

    void SetLinearVelocity(entt::entity entity, const glm::vec3& velocity);
    void AddForce(entt::entity entity, const glm::vec3& force);

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

    float m_accumulator = 0.0f;
    const float m_fixedDt = 1.0f / 60.0f;
};