#pragma once
#include <entt/entity/registry.hpp>
#include "TriggerBoxComponent.h"

namespace EngineFramework
{
    class TriggerSystem
    {
    public:
        // This is called every frame during simulation.
        static void Update(entt::registry& registry);

        /*
            Gets an AABB from the triggerboxcomponent, and writes to it before returning.
            \param parentTransform
            The transform component of the entity this triggerbox is attached to.
        */
        static std::pair<glm::vec3, glm::vec3> GetAABB(TriggerBoxComponent& triggerBox, const TransformComponent& parent_transform);
    };
}