#include "System_Physics.hpp"

#ifdef USE_JOLT
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#endif

namespace EngineFramework {

#ifdef USE_JOLT
    // Install callbacks for Jolt
    static void InstallJoltCallbacks()
    {
        JPH::Trace = TraceImpl;
#ifdef JPH_ENABLE_ASSERTS
        JPH::AssertFailed = AssertFailedImpl;
#endif
    }
#endif

    PhysicsSystem::PhysicsSystem()
    {
        s_instance = this;
    }

    PhysicsSystem::~PhysicsSystem()
    {
#ifdef USE_JOLT
        if (m_physics_system)
        {
            // Unregister the factory before shutting down
            JPH::UnregisterTypes();
            JPH::Factory::sInstance = nullptr;
        }
#endif
        s_instance = nullptr;
    }

    PhysicsSystem& PhysicsSystem::Get()
    {
        return *s_instance;
    }

    void PhysicsSystem::Initialize()
    {
#ifdef USE_JOLT
        // Install callbacks
        InstallJoltCallbacks();

        // Register allocation hook
        JPH::RegisterDefaultAllocator();

        // Create a factory
        JPH::Factory::sInstance = new JPH::Factory();

        // Register all Jolt physics types
        JPH::RegisterTypes();

        // Create temp allocator
        m_temp_allocator = std::make_unique<JPH::TempAllocatorImpl>(10 * 1024 * 1024); // 10 MB

        // Create job system
        m_job_system = std::make_unique<JPH::JobSystemThreadPool>(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, std::thread::hardware_concurrency() - 1);

        // Create mapping table from object layer to broadphase layer
        m_broad_phase_layer_interface = std::make_unique<BPLayerInterfaceImpl>();

        // Create filters
        m_object_vs_broadphase_layer_filter = std::make_unique<ObjectVsBroadPhaseLayerFilterImpl>();
        m_object_vs_object_layer_filter = std::make_unique<ObjectLayerPairFilterImpl>();

        // Create the actual physics system
        m_physics_system = std::make_unique<JPH::PhysicsSystem>();
        m_physics_system->Init(
            10240,  // max bodies
            0,      // num body mutexes
            65536,  // max body pairs
            65536,  // max contact constraints
            *m_broad_phase_layer_interface,
            *m_object_vs_broadphase_layer_filter,
            *m_object_vs_object_layer_filter
        );

        // Set gravity
        m_physics_system->SetGravity(JPH::Vec3(0, -9.81f, 0));

        LOGI("[PhysicsSystem] Initialized with Jolt Physics");
#else
        LOGI("[PhysicsSystem] Initialized (no physics backend compiled)");
#endif
    }

    void PhysicsSystem::Update(float delta_time)
    {
#ifdef USE_JOLT
        if (!m_physics_system) return;

        // Fixed timestep accumulator
        m_accumulator += delta_time;

        while (m_accumulator >= m_fixed_dt)
        {
            // Step the physics world
            m_physics_system->Update(m_fixed_dt, 1, m_temp_allocator.get(), m_job_system.get());
            m_accumulator -= m_fixed_dt;
        }
#endif
    }

    void PhysicsSystem::PostUpdate(float /*delta_time*/)
    {
        // No component syncing for now - just step the physics
        // You can add transform syncing later when you have components set up
    }

    /*
    void PhysicsSystem::SetRegistry(entt::registry* registry)
    {
        m_registry = registry;
    }
    */

    void PhysicsSystem::CreateBoxBody(entt::entity entity, const glm::vec3& position, const glm::vec3& size, bool isStatic)
    {
#ifdef USE_JOLT
        if (!m_physics_system) return;

        // Create a box shape
        JPH::BoxShapeSettings box_settings(JPH::Vec3(size.x * 0.5f, size.y * 0.5f, size.z * 0.5f));
        JPH::ShapeSettings::ShapeResult box_shape_result = box_settings.Create();
        JPH::ShapeRefC box_shape = box_shape_result.Get();

        // Create body creation settings
        JPH::BodyCreationSettings body_settings(
            box_shape,
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