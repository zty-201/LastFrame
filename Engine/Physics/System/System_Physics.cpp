#include "pch.hpp" // if you use a precompiled header; else remove
#include "System_Physics.hpp"

// If you need the types inside this file later:
// #include "Engine/ECS/Component/TransformComponent.hpp"
// #include "Engine/ECS/Component/PhysicsComponent.hpp"
// #include "Engine/ECS/Component/ColliderComponent.hpp"
// #include "CoreEngine.hpp" // if you fetch registry via CoreEngine

namespace EngineFramework {

    PhysicsSystem::PhysicsSystem() {
        s_instance = this;
    }

    PhysicsSystem& PhysicsSystem::Get() {
        return *s_instance;
    }

    void PhysicsSystem::Initialize() {
        // TODO: set up physics backend (e.g., Jolt registration) later
    }

    void PhysicsSystem::Update(float delta_time) {
        // TODO: fixed-step scaffold (leave empty for now)
        m_accumulator += delta_time;
        while (m_accumulator >= m_fixed_dt) {
            // TODO: integrate / collide or call Jolt step
            m_accumulator -= m_fixed_dt;
        }
    }

    void PhysicsSystem::PostUpdate(float /*delta_time*/) {
        // TODO: any late sync (e.g., debug draw) later
    }

} // namespace EngineFramework
