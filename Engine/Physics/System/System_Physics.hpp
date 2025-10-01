#pragma once
#include "ISystem.hpp"
#include <entt/entity/registry.hpp>
#include <iostream>
#include <memory>
#include <unordered_map>
#include <cstdarg>  // for va_list, va_start, va_end
#include <cstdio>   // for vsnprintf

#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>

#ifdef USE_JOLT
#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>
#endif

// forward declares (avoid heavy includes here)
struct TransformComponent;
struct PhysicsComponent;
struct ColliderComponent;

namespace EngineFramework {

#ifdef USE_JOLT
    // Callback for traces, connect this to your own trace function if you have one
    static void TraceImpl(const char* inFMT, ...)
    {
        // Format the message
        va_list list;
        va_start(list, inFMT);
        char buffer[1024];
        vsnprintf(buffer, sizeof(buffer), inFMT, list);
        va_end(list);

        // Print to the TTY
        std::cout << buffer << std::endl;
    }

#ifdef JPH_ENABLE_ASSERTS
    // Callback for asserts, connect this to your own assert handler if you have one
    static bool AssertFailedImpl(const char* inExpression, const char* inMessage, const char* inFile, unsigned int inLine)
    {
        // Print to the TTY
        std::cout << inFile << ":" << inLine << ": (" << inExpression << ") " << (inMessage != nullptr ? inMessage : "") << std::endl;

        // Breakpoint
        return true;
    };
#endif // JPH_ENABLE_ASSERTS

    // Layer that objects can be in, determines which other objects it can collide with
    // Typically you at least want to have 1 layer for moving bodies and 1 layer for static bodies, but you can have more
    // layers if you want. E.g. you could have a layer for high detail collision (which is not used by the physics simulation
    // but only if you do collision testing).
    namespace Layers
    {
        static constexpr JPH::ObjectLayer NON_MOVING = 0;
        static constexpr JPH::ObjectLayer MOVING = 1;
        static constexpr JPH::ObjectLayer NUM_LAYERS = 2;
    };

    /// Class that determines if two object layers can collide
    class ObjectLayerPairFilterImpl : public JPH::ObjectLayerPairFilter
    {
    public:
        virtual bool ShouldCollide(JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2) const override
        {
            switch (inObject1)
            {
            case Layers::NON_MOVING:
                return inObject2 == Layers::MOVING; // Non moving only collides with moving
            case Layers::MOVING:
                return true; // Moving collides with everything
            default:
                JPH_ASSERT(false);
                return false;
            }
        }
    };

    // Each broadphase layer results in a separate bounding volume tree in the broad phase. You at least want to have
    // a layer for non-moving and moving objects to avoid having to update a tree full of static objects every frame.
    // You can have a 1-on-1 mapping between object layers and broadphase layers (like in this example) but you can
    // also group multiple object layers that have the same collision rules into a single broadphase layer.
    namespace BroadPhaseLayers
    {
        static constexpr JPH::BroadPhaseLayer NON_MOVING(0);
        static constexpr JPH::BroadPhaseLayer MOVING(1);
        static constexpr unsigned int NUM_LAYERS(2);
    };

    // BroadPhaseLayerInterface implementation
    // This defines a mapping between object and broadphase layers.
    class BPLayerInterfaceImpl final : public JPH::BroadPhaseLayerInterface
    {
    public:
        BPLayerInterfaceImpl()
        {
            // Create a mapping table from object to broad phase layer
            mObjectToBroadPhase[Layers::NON_MOVING] = BroadPhaseLayers::NON_MOVING;
            mObjectToBroadPhase[Layers::MOVING] = BroadPhaseLayers::MOVING;
        }

        virtual unsigned int GetNumBroadPhaseLayers() const override
        {
            return BroadPhaseLayers::NUM_LAYERS;
        }

        virtual JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override
        {
            JPH_ASSERT(inLayer < Layers::NUM_LAYERS);
            return mObjectToBroadPhase[inLayer];
        }

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
        virtual const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const override
        {
            switch ((JPH::BroadPhaseLayer::Type)inLayer)
            {
            case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::NON_MOVING:	return "NON_MOVING";
            case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::MOVING:		return "MOVING";
            default:													JPH_ASSERT(false); return "INVALID";
            }
        }
#endif // JPH_EXTERNAL_PROFILE || JPH_PROFILE_ENABLED

    private:
        JPH::BroadPhaseLayer mObjectToBroadPhase[Layers::NUM_LAYERS];
    };

    /// Class that determines if an object layer can collide with a broadphase layer
    class ObjectVsBroadPhaseLayerFilterImpl : public JPH::ObjectVsBroadPhaseLayerFilter
    {
    public:
        virtual bool ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const override
        {
            switch (inLayer1)
            {
            case Layers::NON_MOVING:
                return inLayer2 == BroadPhaseLayers::MOVING;
            case Layers::MOVING:
                return true;
            default:
                JPH_ASSERT(false);
                return false;
            }
        }
    };

#endif // USE_JOLT

    class PhysicsSystem final : public ISystem {
    public:
        PhysicsSystem();                 // construct but do nothing heavy
        ~PhysicsSystem();               // cleanup Jolt

        // optional singleton-ish accessor (mirror your LevelEditor pattern if you use one)
        static PhysicsSystem& Get();

        // Physics world management
        void CreateBoxBody(entt::entity entity, const glm::vec3& position, const glm::vec3& size, bool isStatic = false);
        void CreateSphereBody(entt::entity entity, const glm::vec3& position, float radius, bool isStatic = false);
        void CreateCapsuleBody(entt::entity entity, const glm::vec3& position, float height, float radius, bool isStatic = false);

        void RemoveBody(entt::entity entity);
        void SetBodyPosition(entt::entity entity, const glm::vec3& position);
        void SetBodyRotation(entt::entity entity, const glm::quat& rotation);
        void AddForce(entt::entity entity, const glm::vec3& force);
        void AddImpulse(entt::entity entity, const glm::vec3& impulse);

        glm::vec3 GetBodyPosition(entt::entity entity) const;
        glm::quat GetBodyRotation(entt::entity entity) const;
        glm::vec3 GetBodyVelocity(entt::entity entity) const;

        // Access to the registry (you might want to set this via constructor or CoreEngine)
        void SetRegistry(entt::registry* registry);

    private:
        // ISystem overrides
        void Initialize() override;
        void Update(float delta_time) override;
        void PostUpdate(float delta_time) override;

        // Internal state (timestepping scaffold)
        float m_accumulator{ 0.f };
        const float m_fixed_dt{ 1.f / 60.f };

        // Registry reference
        entt::registry* m_registry{ nullptr };

        // Static ptr for Get()
        static inline PhysicsSystem* s_instance{ nullptr };

#ifdef USE_JOLT
        // Jolt Physics World
        std::unique_ptr<JPH::TempAllocatorImpl> m_temp_allocator;
        std::unique_ptr<JPH::JobSystemThreadPool> m_job_system;
        std::unique_ptr<BPLayerInterfaceImpl> m_broad_phase_layer_interface;
        std::unique_ptr<ObjectVsBroadPhaseLayerFilterImpl> m_object_vs_broadphase_layer_filter;
        std::unique_ptr<ObjectLayerPairFilterImpl> m_object_vs_object_layer_filter;
        std::unique_ptr<JPH::PhysicsSystem> m_physics_system;

        // Helper to convert between glm and Jolt vectors
        JPH::RVec3 ToJolt(const glm::vec3& v) const { return JPH::RVec3(v.x, v.y, v.z); }
        JPH::Quat ToJolt(const glm::quat& q) const { return JPH::Quat(q.x, q.y, q.z, q.w); }
        glm::vec3 FromJolt(const JPH::RVec3& v) const { return glm::vec3(v.GetX(), v.GetY(), v.GetZ()); }
        glm::quat FromJolt(const JPH::Quat& q) const { return glm::quat(q.GetW(), q.GetX(), q.GetY(), q.GetZ()); }

        // Entity to BodyID mapping
        std::unordered_map<entt::entity, JPH::BodyID> m_entity_to_body;
        std::unordered_map<JPH::BodyID, entt::entity> m_body_to_entity;
#endif
    };

} // namespace EngineFramework