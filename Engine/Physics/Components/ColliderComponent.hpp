#pragma once
#include <glm/vec3.hpp>

struct ColliderComponent {
    enum class Shape { Box, Sphere, Capsule };
    Shape shape{Shape::Box};

    // Box
    glm::vec3 size{1.0f};          // full extents

    // Sphere
    float radius{0.5f};

    // Capsule
    float height{1.0f};            // cylinder height (no caps)
    float cap_radius{0.5f};

    // Common flags/material
    bool  is_static{false};
    float friction{0.5f};
    float restitution{0.0f};
    float density{1.0f};           // might not need
    bool  is_trigger{false};       
    int   layer{1};                // for your Layers::MOVING/NON_MOVING mapping
};


