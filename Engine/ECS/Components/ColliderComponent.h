#pragma once

#ifndef COLLIDER_COMPONENT
#define COLLIDER_COMPONENT

#include <glm/glm.hpp>

struct ColliderComponent
{
    // what shape to create in Jolt
    enum class Shape
    {
        Box,
        Sphere,
        Capsule
    };

    Shape shape = Shape::Box;

    // -------- shape data --------
    // Box: use full extents (easier to reason about in editor)
    glm::vec3 size{ 1.0f, 1.0f, 1.0f };    // width, height, depth

    // Sphere:
    float radius = 0.5f;

    // Capsule:
    float height = 1.0f;      // full capsule height (cylinder + 2 hemispheres)
    float capRadius = 0.25f;

    // -------- physics flags / material --------
    bool  isStatic = false;   // create static vs dynamic body
    bool  isTrigger = false;   // for later when you do sensor bodies
    float friction = 0.5f;
    float restitution = 0.0f;
    float density = 1.0f;

    // collision filtering (for later)
    int   layer = 1;
};

#endif // COLLIDER_COMPONENT