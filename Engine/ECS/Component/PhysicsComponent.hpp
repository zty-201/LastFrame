#pragma once
#include "IGameComponent.hpp"
#include <glm/vec3.hpp>

struct PhysicsComponent : public IGameComponent {
    // linear
    glm::vec3 velocity{ 0.0f };
    glm::vec3 acceleration{ 0.0f };
    // angular (optional if you don’t need it yet)
    glm::vec3 angularVelocity{ 0.0f };

    float mass{ 1.0f };     // kg
    float invMass{ 1.0f };  // 1/mass; 0 means static/immovable
    float linearDamping{ 0.2f };
    float angularDamping{ 0.2f };
    float restitution{ 0.2f };
    float friction{ 0.5f };
};
