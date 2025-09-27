#pragma once
#include "ISystem.hpp"
#include <memory>
#include <cstdint>
#include <entt/entity/registry.hpp>

// forward declares (avoid heavy includes here)
struct TransformComponent;
struct PhysicsComponent;
struct ColliderComponent;

namespace JPH {
    class PhysicsSystem;
    class JobSystem;
    class TempAllocator;
    class BodyInterface;

    class BroadPhaseLayerInterface;
    class ObjectLayerPairFilter;
    class ObjectVsBroadPhaseLayerFilter;
}

// Your game’s simple layer model (adjust as needed)
enum class PhysicsLayer : uint8_t { NON_MOVING = 0, MOVING = 1 };
enum class BroadPhaseLayerTag : uint8_t { NON_MOVING = 0, MOVING = 1 };

namespace EngineFramework {

    class PhysicsSystem final : public ISystem {
    public:
        PhysicsSystem();                 // construct
        static PhysicsSystem& Get();

        // ISystem interface
        void Initialize() override;         // create Jolt world, allocators, etc.
        void Update(float delta_time) override;     // fixed-step advance
        void PostUpdate(float delta_time) override; // sync poses -> ECS, debug draw, etc.

    private:
        // ---- Fixed-step timing scaffold
        float m_accumulator{ 0.f };
        const float m_fixed_dt{ 1.f / 60.f };

        // ---- Jolt core
        std::unique_ptr<JPH::TempAllocator>  m_temp_alloc;   // scratch allocator for update
        std::unique_ptr<JPH::JobSystem>      m_job_system;   // worker threads
        std::unique_ptr<JPH::PhysicsSystem>  m_jolt;         // the world
        JPH::BodyInterface* m_bodies{ nullptr }; // cached convenience ptr

        // ---- Collision layer routing
        std::unique_ptr<JPH::BroadPhaseLayerInterface>    m_bp_layers;
        std::unique_ptr<JPH::ObjectVsBroadPhaseLayerFilter> m_obj_vs_bp;
        std::unique_ptr<JPH::ObjectLayerPairFilter>       m_obj_pair;
        
        // ---- Singleton backing
        static inline PhysicsSystem* s_instance{ nullptr };
    };

} // namespace EngineFramework
