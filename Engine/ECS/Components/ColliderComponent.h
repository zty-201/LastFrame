#pragma once
#include <glm/glm.hpp>

#ifndef COLLIDER_COMPONENT
#define COLLIDER_COMPONENT

enum class ColliderShapeType
{
    Box,
    Sphere,
    Capsule
};

struct ColliderComponent
{
    ColliderShapeType shapeType = ColliderShapeType::Box;

    // Box data
    glm::vec3 halfExtents{ 0.5f, 0.5f, 0.5f }; // x,y,z half sizes

    // Sphere data
    float radius = 0.5f;

    // Capsule data
    float capsuleHalfHeight = 0.5f; // half-height of the center cylinder
    float capsuleRadius = 0.25f;
};
#endif // COLLIDER_COMPONENT