#pragma once
#include "ISystem.hpp"
#include <entt/entity/registry.hpp>

// forward declares (avoid heavy includes here)
struct TransformComponent;
struct PhysicsComponent;
struct ColliderComponent;

namespace EngineFramework {

    class PhysicsSystem final : public ISystem {
    public:
        PhysicsSystem();                 // construct but do nothing heavy

        // optional singleton-ish accessor (mirror your LevelEditor pattern if you use one)
        static PhysicsSystem& Get();

    private:
        // ISystem overrides (empty for now)
        void Initialize() override;
        void Update(float delta_time) override;
        void PostUpdate(float delta_time) override;

        // Internal state (timestepping scaffold only)
        float m_accumulator{ 0.f };
        const float m_fixed_dt{ 1.f / 60.f };

        // If you keep a global registry in CoreEngine, you can fetch it inside methods.
        // Otherwise, add a reference here and pass it in the ctor.

        // Static ptr for Get()
        static inline PhysicsSystem* s_instance{ nullptr };
    };

} // namespace EngineFramework
