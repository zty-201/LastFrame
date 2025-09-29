#pragma once
#include "ISystem.hpp"
#include <entt/entity/registry.hpp>
#include <iostream>
#include <memory>
#include <unordered_map>
#include <cstdarg>
#include <cstdio>

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

struct TransformComponent;
struct PhysicsComponent;
struct ColliderComponent;

namespace EngineFramework {

#ifdef USE_JOLT
    inline void TraceImpl(const char* inFMT, ...)
    {
        va_list list;
        va_start(list, inFMT);
        char buffer[1024];
        vsnprintf(buffer, sizeof(buffer), inFMT, list);
        va_end(list);
        std::cout << buffer << std::endl;
    }

#ifdef JPH_ENABLE_ASSERTS
    inline bool AssertFailedImpl(const char* inExpression, const char* inMessage, const char* inFile, unsigned int inLine)
    {
        std::cout << inFile << ":" << inLine << ": (" << inExpression << ") " << (inMessage != nullptr ? inMessage : "") << std::endl;
        return true;
    };
#endif

    namespace Layers
    {
        static constexpr JPH::ObjectLayer NON_MOVING = 0;
        static constexpr JPH::ObjectLayer MOVING = 1;
        static constexpr JPH::ObjectLayer NUM_LAYERS = 2;
    };

    class ObjectLayerPairFilterImpl : public JPH::ObjectLayerPairFilter
    {
    public:
        virtual bool ShouldCollide(JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2) const override
        {
            switch (inObject1)
            {
            case Layers::NON_MOVING:
                return inObject2 == Layers::MOVING;
            case Layers::MOVING:
                return true;
            default:
                JPH_ASSERT(false);
                return false;
            }
        }
    };

    namespace BroadPhaseLayers
    {
        static constexpr JPH::BroadPhaseLayer NON_MOVING(0);
        static constexpr JPH::BroadPhaseLayer MOVING(1);
        static constexpr unsigned int NUM_LAYERS(2);
    };

    class BPLayerInterfaceImpl final : public JPH::BroadPhaseLayerInterface
    {
    public:
        BPLayerInterfaceImpl()
        {
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
            case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::NON_MOVING: return "NON_MOVING";
            case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::MOVING: return "MOVING";
            default: JPH_ASSERT(false); return "INVALID";
            }
        }
#endif

    private:
        JPH::BroadPhaseLayer mObjectToBroadPhase[Layers::NUM_LAYERS];
    };

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
#endif

    class PhysicsSystem final : public ISystem {
    public:
        PhysicsSystem();
        ~PhysicsSystem();

        static PhysicsSystem& Get();

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

        void SetRegistry(entt::registry* registry);

    private:
        void Initialize() override;
        void Update(float delta_time) override;
        void PostUpdate(float delta_time) override;

        float m_accumulator{ 0.f };
        const float m_fixed_dt{ 1.f / 60.f };
        entt::registry* m_registry{ nullptr };
        static inline PhysicsSystem* s_instance{ nullptr };

#ifdef USE_JOLT
        std::unique_ptr<JPH::TempAllocatorImpl> m_temp_allocator;
        std::unique_ptr<JPH::JobSystemThreadPool> m_job_system;
        std::unique_ptr<BPLayerInterfaceImpl> m_broad_phase_layer_interface;
        std::unique_ptr<ObjectVsBroadPhaseLayerFilterImpl> m_object_vs_broadphase_layer_filter;
        std::unique_ptr<ObjectLayerPairFilterImpl> m_object_vs_object_layer_filter;
        std::unique_ptr<JPH::PhysicsSystem> m_physics_system;

        JPH::RVec3 ToJolt(const glm::vec3& v) const { return JPH::RVec3(v.x, v.y, v.z); }
        JPH::Quat ToJolt(const glm::quat& q) const { return JPH::Quat(q.x, q.y, q.z, q.w); }
        glm::vec3 FromJolt(const JPH::RVec3& v) const { return glm::vec3(v.GetX(), v.GetY(), v.GetZ()); }
        glm::quat FromJolt(const JPH::Quat& q) const { return glm::quat(q.GetW(), q.GetX(), q.GetY(), q.GetZ()); }

        std::unordered_map<entt::entity, JPH::BodyID> m_entity_to_body;
        std::unordered_map<JPH::BodyID, entt::entity> m_body_to_entity;
#endif
    };

} // namespace EngineFramework