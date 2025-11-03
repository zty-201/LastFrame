#include "System_Physics.hpp"

#include <algorithm> // std::min
#include <cmath>
#include <thread>

#ifdef USE_JOLT
// Core Jolt setup
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Core/TempAllocator.h>

// Shapes / body creation
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>

// CRITICAL: This include was missing!
#include <Jolt/Physics/PhysicsSettings.h>

// System & Queries
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/NarrowPhaseQuery.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Body/Body.h>
#endif

namespace EngineFramework {

    // ---------------- Singleton plumbing ----------------
    PhysicsSystem* PhysicsSystem::s_instance = nullptr;

    PhysicsSystem& PhysicsSystem::GetInstance() {
        static PhysicsSystem g;
        return g;
    }

    PhysicsSystem::PhysicsSystem() {
        s_instance = this;
    }

    PhysicsSystem::~PhysicsSystem() {
        Shutdown(); // Ensure cleanup happens
        s_instance = nullptr;
    }

    // --------------- ISystem: Initialize -----------------
    void PhysicsSystem::Initialize() {
#ifdef USE_JOLT
        // Allocator + factory + type registry
        JPH::RegisterDefaultAllocator();
        JPH::Factory::sInstance = new JPH::Factory();
        JPH::RegisterTypes();

        // Temporary allocator (scratch) and job system (threads)
        m_temp_alloc = std::make_unique<JPH::TempAllocatorImpl>(8u * 1024u * 1024u); // 8MB scratch
        const int num_worker_threads =
            std::max(1, int(std::thread::hardware_concurrency()) - 1);
        m_job_system = std::make_unique<JPH::JobSystemThreadPool>(JPH::cMaxPhysicsJobs,
            JPH::cMaxPhysicsBarriers,
            num_worker_threads);

        // Physics world
        m_world = std::make_unique<JPH::PhysicsSystem>();

        // Tuning (HelloWorld-style numbers)
        const JPH::uint cMaxBodies = 4096;
        const JPH::uint cNumBodyMutexes = 0;     // 0 = default internal
        const JPH::uint cMaxBodyPairs = 4096;
        const JPH::uint cMaxContactConstraints = 4096;

        m_world->Init(cMaxBodies,
            cNumBodyMutexes,
            cMaxBodyPairs,
            cMaxContactConstraints,
            m_bp,
            m_obj_vs_bp,
            m_obj_pair);

        // Gravity
        m_world->SetGravity(JPH::Vec3(0.0f, -9.81f, 0.0f));

        // Create a static ground box
        {
            JPH::BoxShapeSettings box(JPH::Vec3(50.0f, 1.0f, 50.0f)); // half-extents
            JPH::ShapeSettings::ShapeResult shape_result = box.Create();
            JPH::ShapeRefC shape = shape_result.Get();

            JPH::BodyCreationSettings s(shape,
                JPH::RVec3(0.0, -1.0, 0.0), // put top at y=0
                JPH::Quat::sIdentity(),
                JPH::EMotionType::Static,
                Layers::NON_MOVING);

            auto& bi = m_world->GetBodyInterface();
            m_ground = bi.CreateAndAddBody(s, JPH::EActivation::DontActivate);
        }

        // Create a dynamic sphere that falls onto the ground
        {
            JPH::SphereShapeSettings sph(0.5f); // radius
            JPH::ShapeSettings::ShapeResult shape_result = sph.Create();
            JPH::ShapeRefC shape = shape_result.Get();

            JPH::BodyCreationSettings s(shape,
                JPH::RVec3(0.0, 3.0, 0.0),
                JPH::Quat::sIdentity(),
                JPH::EMotionType::Dynamic,
                Layers::MOVING);

            // Give it mass (and auto inertia)
            s.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
            s.mMassPropertiesOverride.mMass = 1.0f;

            auto& bi = m_world->GetBodyInterface();
            m_ball = bi.CreateAndAddBody(s, JPH::EActivation::Activate);
        }
#endif // USE_JOLT
    }

    // ---------------- ISystem: Update --------------------
    void PhysicsSystem::Update(float delta_time) {
#ifdef USE_JOLT
        if (!m_world) return;

        // Clamp hostile frame spikes and accumulator growth
        delta_time = std::min(delta_time, 0.25f);
        m_accum = std::min(m_accum + delta_time, 0.5f);

        // Step the world in fixed increments
        int substeps = 0;
        const int max_substeps = 4;
        while (m_accum >= m_fixed_dt && substeps++ < max_substeps) {
            // Note: collision_steps = 1 is typical for most games
            m_world->Update(m_fixed_dt, 1, m_temp_alloc.get(), m_job_system.get());
            m_accum -= m_fixed_dt;
        }
#else
        (void)delta_time;
#endif
    }

    // -------------- ISystem: PostUpdate ------------------
    void PhysicsSystem::PostUpdate(float delta_time) {
        // Hook for debug draw / overlays if you wire Jolt's debug renderer later
        (void)delta_time;
    }

    // --------------------- Shutdown ----------------------
#ifdef USE_JOLT
    static void DestroyBodyIfValid(JPH::PhysicsSystem& world, JPH::BodyID id) {
        if (id == JPH::BodyID()) return; // Check if default/invalid
        auto& bi = world.GetBodyInterface();
        bi.RemoveBody(id);
        bi.DestroyBody(id);
    }
#endif

    void PhysicsSystem::ClearWorld() {
#ifdef USE_JOLT
        if (!m_world) return;
        DestroyBodyIfValid(*m_world, m_ball);
        DestroyBodyIfValid(*m_world, m_ground);
        m_ball = JPH::BodyID();
        m_ground = JPH::BodyID();
#endif
    }

    void PhysicsSystem::Shutdown() {
#ifdef USE_JOLT
        if (m_world) {
            ClearWorld();
            m_world.reset();
        }
        m_job_system.reset();
        m_temp_alloc.reset();

        // Tear down type registry & factory
        JPH::UnregisterTypes();
        if (JPH::Factory::sInstance != nullptr) {
            delete JPH::Factory::sInstance;
            JPH::Factory::sInstance = nullptr;
        }
#endif
    }

    // -------------------- Queries ------------------------
#ifdef USE_JOLT
    PhysicsSystem::RayHit PhysicsSystem::RayCastClosest(float ox, float oy, float oz,
        float dx, float dy, float dz,
        float max_distance) const
    {
        RayHit out{};

        if (!m_world) return out;

        // Normalize direction; guard zero length
        const float len = std::sqrt(dx * dx + dy * dy + dz * dz);
        if (len < 1e-6f) return out; // Invalid direction

        const float inv = 1.0f / len;
        const JPH::RVec3 origin(ox, oy, oz);
        const JPH::Vec3 direction(dx * inv, dy * inv, dz * inv);

        JPH::RRayCast ray;
        ray.mOrigin = origin;
        ray.mDirection = direction * max_distance;

        JPH::RayCastResult result;
        const bool hit = m_world->GetNarrowPhaseQuery().CastRay(ray, result);

        if (hit) {
            out.hit = true;
            out.t = result.mFraction * max_distance;

            const JPH::RVec3 p = origin + JPH::RVec3(direction) * out.t;
            out.px = static_cast<float>(p.GetX());
            out.py = static_cast<float>(p.GetY());
            out.pz = static_cast<float>(p.GetZ());

            // Get the body to retrieve the surface normal
            auto& body_lock_interface = m_world->GetBodyLockInterface();
            JPH::BodyLockRead lock(body_lock_interface, result.mBodyID);
            if (lock.Succeeded()) {
                const JPH::Body& body = lock.GetBody();

                // Get surface normal using shape
                JPH::Vec3 normal = body.GetWorldSpaceSurfaceNormal(result.mSubShapeID2,
                    JPH::RVec3(out.px, out.py, out.pz));

                out.nx = normal.GetX();
                out.ny = normal.GetY();
                out.nz = normal.GetZ();
            }

            // If you later map BodyID -> entity, fill it here from result.mBodyID
            out.entity_u32 = result.mBodyID.GetIndex();
        }

        return out;
    }

    void PhysicsSystem::SetLinearVelocity(unsigned entity, const JPH::Vec3& v) {
        if (!m_world) return;
        // TODO: Implement entity -> BodyID mapping
        // For now, stub implementation
        (void)entity;
        (void)v;
    }

    void PhysicsSystem::SetAngularVelocity(unsigned entity, const JPH::Vec3& w) {
        if (!m_world) return;
        // TODO: Implement entity -> BodyID mapping
        (void)entity;
        (void)w;
    }

    JPH::Vec3 PhysicsSystem::GetLinearVelocity(unsigned entity) const {
        if (!m_world) return JPH::Vec3::sZero();
        // TODO: Implement entity -> BodyID mapping
        (void)entity;
        return JPH::Vec3::sZero();
    }

    JPH::Vec3 PhysicsSystem::GetAngularVelocity(unsigned entity) const {
        if (!m_world) return JPH::Vec3::sZero();
        // TODO: Implement entity -> BodyID mapping
        (void)entity;
        return JPH::Vec3::sZero();
    }
#endif // USE_JOLT

} // namespace EngineFramework