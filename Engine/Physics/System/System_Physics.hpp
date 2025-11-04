#ifndef ENGINE_PHYSICS_SYSTEM_HPP
#define ENGINE_PHYSICS_SYSTEM_HPP
#pragma once

#include "ISystem.hpp"
#include <memory>
#include <unordered_map>

// Jolt includes only if enabled
#ifdef USE_JOLT
#include <Jolt/Jolt.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/JobSystem.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Body/BodyID.h>
#include <Jolt/Physics/Body/BodyLockInterface.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>
#endif

namespace EngineFramework {

    class PhysicsSystem final : public ISystem {
    public:
        // Pseudo-singleton (CoreEngine creates exactly one; we keep a back-pointer)
        static PhysicsSystem& GetInstance();

        PhysicsSystem();
        ~PhysicsSystem() override;

        // Timestep control (public API you may tweak at runtime)
        void SetFixedDelta(float step) { m_fixed_dt = step; }
        float GetFixedDelta() const { return m_fixed_dt; }

        // Cleanup methods (public so you can call from CoreEngine)
        void ClearWorld();
        void Shutdown();

#ifdef USE_JOLT
        // Simple query API you can call from gameplay
        struct RayHit {
            bool hit{ false };
            unsigned int entity_u32{ 0 }; // BodyID index if hit
            float t{ 0.0f };
            float nx{ 0 }, ny{ 0 }, nz{ 0 }; // Normal
            float px{ 0 }, py{ 0 }, pz{ 0 }; // Position
        };

        RayHit RayCastClosest(float ox, float oy, float oz,
            float dx, float dy, float dz,
            float max_distance = 1000.0f) const;

        // Velocity setters/getters (stub for now - needs entity mapping)
        void SetLinearVelocity(unsigned entity, const JPH::Vec3& v);
        void SetAngularVelocity(unsigned entity, const JPH::Vec3& w);
        JPH::Vec3 GetLinearVelocity(unsigned entity) const;
        JPH::Vec3 GetAngularVelocity(unsigned entity) const;
#endif

    private:
        // ===== ISystem interface (private by design; only CoreEngine can call) =====
        void Initialize() override;
        void Update(float delta_time) override;
        void PostUpdate(float delta_time) override;

#ifdef USE_JOLT
        // ---------- Minimal layer/filter setup like Jolt HelloWorld ----------
        struct Layers {
            static constexpr JPH::ObjectLayer NON_MOVING = 0;
            static constexpr JPH::ObjectLayer MOVING = 1;
            static constexpr JPH::ObjectLayer NUM_LAYERS = 2;
        };

        struct BroadLayers {
            static constexpr JPH::BroadPhaseLayer NON_MOVING{ 0 };
            static constexpr JPH::BroadPhaseLayer MOVING{ 1 };
            static constexpr JPH::uint NUM{ 2 };
        };

        class BPInterface final : public JPH::BroadPhaseLayerInterface {
        public:
            BPInterface() {
                m_map[Layers::NON_MOVING] = BroadLayers::NON_MOVING;
                m_map[Layers::MOVING] = BroadLayers::MOVING;
            }

            JPH::uint GetNumBroadPhaseLayers() const override {
                return BroadLayers::NUM;
            }

            JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer layer) const override {
                JPH_ASSERT(layer < Layers::NUM_LAYERS);
                return m_map[layer];
            }

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
            const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer layer) const override {
                // FIXED: Proper comparison using BroadPhaseLayer equality
                if (layer == BroadLayers::NON_MOVING)
                    return "NON_MOVING";
                else if (layer == BroadLayers::MOVING)
                    return "MOVING";
                else
                    return "UNKNOWN";
            }
#endif
        private:
            JPH::BroadPhaseLayer m_map[Layers::NUM_LAYERS];
        };

        class ObjVsBPFilter final : public JPH::ObjectVsBroadPhaseLayerFilter {
        public:
            bool ShouldCollide(JPH::ObjectLayer layer, JPH::BroadPhaseLayer bp) const override {
                switch (layer) {
                case Layers::NON_MOVING:
                    return bp == BroadLayers::MOVING;
                case Layers::MOVING:
                    return true;
                default:
                    JPH_ASSERT(false);
                    return false;
                }
            }
        };

        class ObjPairFilter final : public JPH::ObjectLayerPairFilter {
        public:
            bool ShouldCollide(JPH::ObjectLayer a, JPH::ObjectLayer b) const override {
                switch (a) {
                case Layers::NON_MOVING:
                    return b == Layers::MOVING;
                case Layers::MOVING:
                    return true;
                default:
                    JPH_ASSERT(false);
                    return false;
                }
            }
        };

        // ---------- World state ----------
        std::unique_ptr<JPH::TempAllocator> m_temp_alloc;
        std::unique_ptr<JPH::JobSystem>     m_job_system;
        std::unique_ptr<JPH::PhysicsSystem> m_world;

        BPInterface   m_bp{};
        ObjVsBPFilter m_obj_vs_bp{};
        ObjPairFilter m_obj_pair{};

        // Demo bodies (HelloWorld-style: one ground, one sphere)
        JPH::BodyID   m_ground{};
        JPH::BodyID   m_ball{};
#endif

        // ---------- Stepping ----------
        float m_fixed_dt{ 1.0f / 60.0f };
        float m_accum{ 0.0f };

        // ---------- Singleton back-pointer ----------
        static PhysicsSystem* s_instance;
    };

} // namespace EngineFramework

#endif // ENGINE_PHYSICS_SYSTEM_HPP