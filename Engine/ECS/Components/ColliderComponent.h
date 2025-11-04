#ifndef COLLIDING_COMPONENT
#define COLLIDING_COMPONENT

#include <glm/vec3.hpp>

enum class ColliderType : unsigned char {
    Box,
    Sphere
};

struct ColliderComponent {
    ColliderType type = ColliderType::Box;

    // Local offset from the entity transform
    glm::vec3 offset{ 0.0f, 0.0f, 0.0f };

    // Box: if any <= 0, we’ll derive from Transform.scale * 0.5f
    glm::vec3 half_extents{ 0.0f, 0.0f, 0.0f };

    // Sphere: if <= 0, we’ll derive from max(Transform.scale) * 0.5f
    float radius = 0.0f;

    bool dirty = true;     // mark true when edited so physics rebuilds
    bool _has_body = false;// set by PhysicsSystem; don’t touch in gameplay
};

#endif