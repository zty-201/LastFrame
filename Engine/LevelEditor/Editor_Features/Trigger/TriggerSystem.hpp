#pragma once
#include <entt/entity/registry.hpp>

namespace EngineFramework
{
    class TriggerSystem
    {
    public:
        // This is called every frame during simulation.
        static void Update(entt::registry& registry);
    };
}