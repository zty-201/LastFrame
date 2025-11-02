#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif

#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/constants.hpp>
#include <cmath>

#include "pch.hpp"
#include "System_Physics.hpp"
#include "ECS/Components/TransformComponent.h"

#ifdef USE_JOLT
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/Body/BodyLockInterface.h>
#endif

// ---- Quaternion -> Euler (degrees) helper ----
static inline glm::vec3 QuatToEulerDegrees(const glm::quat& q) {
    // roll (Z)
    double sinr_cosp = 2.0 * (q.w * q.z + q.x * q.y);
    double cosr_cosp = 1.0 - 2.0 * (q.y * q.y + q.z * q.z);
    double roll = std::atan2(sinr_cosp, cosr_cosp);

    // pitch (X)
    double sinp = 2.0 * (q.w * q.x - q.z * q.y);
    double pitch = std::abs(sinp) >= 1.0
        ? std::copysign(glm::pi<double>() / 2.0, sinp)
        : std::asin(sinp);

    // yaw (Y)
    double siny_cosp = 2.0 * (q.w * q.y + q.x * q.z);
    double cosy_cosp = 1.0 - 2.0 * (q.x * q.x + q.y * q.y);
    double yaw = std::atan2(siny_cosp, cosy_cosp);

    return glm::degrees(glm::vec3((float)pitch, (float)yaw, (float)roll));
}

namespace EngineFramework {

#ifdef USE_JOLT
    // ---- Install trace/assert callbacks ----
    static void InstallJoltCallbacks()
    {
        JPH::Trace = TraceImpl;
    #ifdef JPH_ENABLE_ASSERTS
        JPH::AssertFailed = AssertFailedImpl;
    #endif
    }
    #endif  // USE_JOLT

    // ---------------------------------------------------------------------------
    // Construction / Destruction
    // ---------------------------------------------------------------------------
    PhysicsSystem::PhysicsSystem()
    {
        s_instance = this;
    }

    PhysicsSystem::~PhysicsSystem()
    {
    #ifdef USE_JOLT
        if (m_physics_system)
        {
            // Proper shutdown order
            JPH::UnregisterTypes();
            delete JPH::Factory::sInstance;
            JPH::Factory::sInstance = nullptr;
        }
    #endif
        s_instance = nullptr;
    }

    PhysicsSystem& PhysicsSystem::Get()
    {
        return *s_instance;
    }

    // ---------------------------------------------------------------------------
    // Initialization
    // ---------------------------------------------------------------------------
    void PhysicsSystem::Initialize()
    {
    #ifdef USE_JOLT
        // Initialise Jolt core systems
        InstallJoltCallbacks();
        JPH::RegisterDefaultAllocator();
        JPH::Factory::sInstance = new JPH::Factory();
        JPH::RegisterTypes();

        // Threading and Memory
        m_temp_allocator = std::make_unique<JPH::TempAllocatorImpl>(10 * 1024 * 1024);
        m_job_system = std::make_unique<JPH::JobSystemThreadPool>(
            JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers,
            std::max(1u, std::thread::hardware_concurrency() - 1));

        // Broadphase / filters
        m_broad_phase_layer_interface = std::make_unique<BPLayerInterfaceImpl>();
        m_object_vs_broadphase_layer_filter = std::make_unique<ObjectVsBroadPhaseLayerFilterImpl>();
        m_object_vs_object_layer_filter = std::make_unique<ObjectLayerPairFilterImpl>();

        // Create physics system
        m_physics_system = std::make_unique<JPH::PhysicsSystem>();
        m_physics_system->Init(
            10240,  // max bodies
            8,      // num body mutexes (for parallelism)
            65536,  // max body pairs
            65536,  // max contact constraints
            *m_broad_phase_layer_interface,
            *m_object_vs_broadphase_layer_filter,
            *m_object_vs_object_layer_filter
        );

        
        m_physics_system->SetGravity(JPH::Vec3(0, -9.81f, 0));

        LOGI("[PhysicsSystem] Initialized with Jolt Physics");
    #else
        LOGI("[PhysicsSystem] Initialized (no physics backend compiled)");
    #endif
    }

    // ---------------------------------------------------------------------------
    // Update Loop
    // ---------------------------------------------------------------------------
    void PhysicsSystem::Update(float delta_time)
    {
    #ifdef USE_JOLT
        if (!m_physics_system) return;

        // Push ECS to Physics
        SyncECS2Physics();

        // Fixed-step simulate Jolt world
        m_accumulator += delta_time;
        while (m_accumulator >= m_fixed_dt)
        {
            m_physics_system->Update(m_fixed_dt, 1, m_temp_allocator.get(), m_job_system.get());
            m_accumulator -= m_fixed_dt;
        }

        // Pull Physics to ECS
        SyncPhysics2ECS();
    #else
        (void)delta_time;
    #endif
    }

    void PhysicsSystem::PostUpdate(float /*delta_time*/)
    {
        // add debug draw or contact events
    }

    // ---------------------------------------------------------------------------
    // Registry
    // ---------------------------------------------------------------------------
    void PhysicsSystem::SetRegistry(entt::registry* registry)
    {
        m_registry = registry;
    }
    
    // ---------------------------------------------------------------------------
    // ECS <-> Physics Sync
    // ---------------------------------------------------------------------------
    void PhysicsSystem::SyncECS2Physics()
    {
#ifdef USE_JOLT
        if (!m_registry || !m_physics_system) return;

        auto view = m_registry->view<TransformComponent>();
        JPH::BodyInterface& bi = m_physics_system->GetBodyInterface();
        const JPH::BodyLockInterface& bli = m_physics_system->GetBodyLockInterface();

        for (auto e : view)
        {
            auto it = m_entity_to_body.find(e);
            if (it == m_entity_to_body.end()) continue;

            const JPH::BodyID id = it->second;

            // Only push to physics if this body is NOT dynamic (avoid fighting the sim)
            bool should_push = false;
            {
                JPH::BodyLockRead lock(bli, id);
                if (lock.Succeeded())
                {
                    const JPH::Body& body = lock.GetBody();
                    should_push = (body.GetMotionType() != JPH::EMotionType::Dynamic);
                }
            }
            if (!should_push) continue;

            const auto& tc = view.get<TransformComponent>(e);

            // Your TransformComponent stores Euler angles (likely degrees) -> convert to quat
            // If your euler is already in radians, remove glm::degrees/ radians conversion accordingly.
            const glm::quat q = glm::quat(glm::radians(tc.rotation));

            bi.SetPositionAndRotation(
                id,
                ToJolt(tc.position),
                ToJolt(q),
                JPH::EActivation::Activate
            );
        }
#endif
    }

    void PhysicsSystem::SyncPhysics2ECS()
    {
#ifdef USE_JOLT
        if (!m_registry || !m_physics_system) return;

        auto view = m_registry->view<TransformComponent>();
        const JPH::BodyLockInterface& bli = m_physics_system->GetBodyLockInterface();

        for (auto e : view)
        {
            auto it = m_entity_to_body.find(e);
            if (it == m_entity_to_body.end()) continue;

            const JPH::BodyID id = it->second;

            JPH::BodyLockRead lock(bli, id);
            if (!lock.Succeeded()) continue;

            const JPH::Body& body = lock.GetBody();

            auto& tc = view.get<TransformComponent>(e);

            // Write back simulated position
            tc.position = FromJolt(body.GetPosition());

            // Convert simulated rotation (quat) back to Euler for your component
            const glm::quat q = FromJolt(body.GetRotation());
            tc.rotation = QuatToEulerDegrees(q);
        }
#endif
    }

    

    // ---------------------------------------------------------------------------
    // Body creation / control (your original helpers)
    // ---------------------------------------------------------------------------
    void PhysicsSystem::CreateBoxBody(entt::entity entity, const glm::vec3& position, const glm::vec3& size, bool isStatic)
    {
    #ifdef USE_JOLT
        if (!m_physics_system) return;

        JPH::BoxShapeSettings box_settings(JPH::Vec3(size.x * 0.5f, size.y * 0.5f, size.z * 0.5f));
        JPH::ShapeRefC box_shape = box_settings.Create().Get();

        JPH::BodyCreationSettings body_settings(
            box_shape,
            ToJolt(position),
            JPH::Quat::sIdentity(),
            isStatic ? JPH::EMotionType::Static : JPH::EMotionType::Dynamic,
            isStatic ? Layers::NON_MOVING : Layers::MOVING
        );

        JPH::BodyInterface& bi = m_physics_system->GetBodyInterface();
        JPH::Body* body = bi.CreateBody(body_settings);

        if (body)
        {
            JPH::BodyID id = body->GetID();
            bi.AddBody(id, JPH::EActivation::Activate);
            m_entity_to_body[entity] = id;
            m_body_to_entity[id] = entity;
        }
    #endif
    }

    void PhysicsSystem::CreateSphereBody(entt::entity entity, const glm::vec3& position, float radius, bool isStatic)
    {
#ifdef USE_JOLT
        if (!m_physics_system) return;

        // Create a sphere shape
        JPH::SphereShapeSettings sphere_settings(radius);
        JPH::ShapeSettings::ShapeResult sphere_shape_result = sphere_settings.Create();
        JPH::ShapeRefC sphere_shape = sphere_shape_result.Get();

        // Create body creation settings
        JPH::BodyCreationSettings body_settings(
            sphere_shape,
            ToJolt(position),
            JPH::Quat::sIdentity(),
            isStatic ? JPH::EMotionType::Static : JPH::EMotionType::Dynamic,
            isStatic ? Layers::NON_MOVING : Layers::MOVING
        );

        // Create the body
        JPH::BodyInterface& body_interface = m_physics_system->GetBodyInterface();
        JPH::Body* body = body_interface.CreateBody(body_settings);

        if (body)
        {
            JPH::BodyID body_id = body->GetID();
            body_interface.AddBody(body_id, JPH::EActivation::Activate);

            // Store mapping
            m_entity_to_body[entity] = body_id;
            m_body_to_entity[body_id] = entity;
        }
#endif
    }

    void PhysicsSystem::CreateCapsuleBody(entt::entity entity, const glm::vec3& position, float height, float radius, bool isStatic)
    {
#ifdef USE_JOLT
        if (!m_physics_system) return;

        // Create a capsule shape (height is the cylinder height, not including the hemispheres)
        JPH::CapsuleShapeSettings capsule_settings(height * 0.5f, radius);
        JPH::ShapeSettings::ShapeResult capsule_shape_result = capsule_settings.Create();
        JPH::ShapeRefC capsule_shape = capsule_shape_result.Get();

        // Create body creation settings
        JPH::BodyCreationSettings body_settings(
            capsule_shape,
            ToJolt(position),
            JPH::Quat::sIdentity(),
            isStatic ? JPH::EMotionType::Static : JPH::EMotionType::Dynamic,
            isStatic ? Layers::NON_MOVING : Layers::MOVING
        );

        // Create the body
        JPH::BodyInterface& body_interface = m_physics_system->GetBodyInterface();
        JPH::Body* body = body_interface.CreateBody(body_settings);

        if (body)
        {
            JPH::BodyID body_id = body->GetID();
            body_interface.AddBody(body_id, JPH::EActivation::Activate);

            // Store mapping
            m_entity_to_body[entity] = body_id;
            m_body_to_entity[body_id] = entity;
        }
#endif
    }

    void PhysicsSystem::RemoveBody(entt::entity entity)
    {
#ifdef USE_JOLT
        if (!m_physics_system) return;

        auto it = m_entity_to_body.find(entity);
        if (it != m_entity_to_body.end())
        {
            JPH::BodyID body_id = it->second;
            JPH::BodyInterface& body_interface = m_physics_system->GetBodyInterface();

            // Remove and destroy the body
            body_interface.RemoveBody(body_id);
            body_interface.DestroyBody(body_id);

            // Remove from mappings
            m_body_to_entity.erase(body_id);
            m_entity_to_body.erase(it);
        }
#endif
    }

    void PhysicsSystem::SetBodyPosition(entt::entity entity, const glm::vec3& position)
    {
#ifdef USE_JOLT
        if (!m_physics_system) return;

        auto it = m_entity_to_body.find(entity);
        if (it != m_entity_to_body.end())
        {
            JPH::BodyInterface& body_interface = m_physics_system->GetBodyInterface();
            body_interface.SetPosition(it->second, ToJolt(position), JPH::EActivation::Activate);
        }
#endif
    }

    void PhysicsSystem::SetBodyRotation(entt::entity entity, const glm::quat& rotation)
    {
#ifdef USE_JOLT
        if (!m_physics_system) return;

        auto it = m_entity_to_body.find(entity);
        if (it != m_entity_to_body.end())
        {
            JPH::BodyInterface& body_interface = m_physics_system->GetBodyInterface();
            body_interface.SetRotation(it->second, ToJolt(rotation), JPH::EActivation::Activate);
        }
#endif
    }

    void PhysicsSystem::AddForce(entt::entity entity, const glm::vec3& force)
    {
#ifdef USE_JOLT
        if (!m_physics_system) return;

        auto it = m_entity_to_body.find(entity);
        if (it != m_entity_to_body.end())
        {
            JPH::BodyInterface& body_interface = m_physics_system->GetBodyInterface();
            body_interface.AddForce(it->second, ToJolt(force));
        }
#endif
    }

    void PhysicsSystem::AddImpulse(entt::entity entity, const glm::vec3& impulse)
    {
#ifdef USE_JOLT
        if (!m_physics_system) return;

        auto it = m_entity_to_body.find(entity);
        if (it != m_entity_to_body.end())
        {
            JPH::BodyInterface& body_interface = m_physics_system->GetBodyInterface();
            body_interface.AddImpulse(it->second, ToJolt(impulse));
        }
#endif
    }

    glm::vec3 PhysicsSystem::GetBodyPosition(entt::entity entity) const
    {
#ifdef USE_JOLT
        if (!m_physics_system) return glm::vec3(0.0f);

        auto it = m_entity_to_body.find(entity);
        if (it != m_entity_to_body.end())
        {
            JPH::BodyInterface& body_interface = m_physics_system->GetBodyInterface();
            return FromJolt(body_interface.GetPosition(it->second));
        }
#endif
        return glm::vec3(0.0f);
    }

    glm::quat PhysicsSystem::GetBodyRotation(entt::entity entity) const
    {
#ifdef USE_JOLT
        if (!m_physics_system) return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

        auto it = m_entity_to_body.find(entity);
        if (it != m_entity_to_body.end())
        {
            JPH::BodyInterface& body_interface = m_physics_system->GetBodyInterface();
            return FromJolt(body_interface.GetRotation(it->second));
        }
#endif
        return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    }

    glm::vec3 PhysicsSystem::GetBodyVelocity(entt::entity entity) const
    {
#ifdef USE_JOLT
        if (!m_physics_system) return glm::vec3(0.0f);

        auto it = m_entity_to_body.find(entity);
        if (it != m_entity_to_body.end())
        {
            JPH::BodyInterface& body_interface = m_physics_system->GetBodyInterface();
            return FromJolt(body_interface.GetLinearVelocity(it->second));
        }
#endif
        return glm::vec3(0.0f);
    }

} // namespace EngineFramework