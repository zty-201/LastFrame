#pragma once
#include "IGameComponent.hpp"
#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>

struct TransformComponent : public IGameComponent {
    glm::vec3 position{ 0.0f, 0.0f, 0.0f };
    glm::quat rotation{ 1.0f, 0.0f, 0.0f, 0.0f };
    glm::vec3 scale{ 1.0f, 1.0f, 1.0f };

    bool dirty{ true };
};
