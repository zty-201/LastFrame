#pragma once

#ifndef COLLIDER_COMPONENT
#define COLLIDER_COMPONENT

#include <glm/glm.hpp>
#include <vector>

enum class ColliderShape
{
    Box = 0,
    Sphere = 1,
    Capsule = 2
};

struct ColliderData
{
    int type = 0;

    //Shape Dimensions
    glm::vec3 boxHalfExtents = { 0.5f, 0.5f, 0.5f };
    float sphereRadius = 0.5f;
    float capsuleHeight = 1.0f;
    float capsuleRadius = 0.25f;

    //Orientation / Offset
    glm::vec3 offsetPosition = { 0.0f, 0.0f, 0.0f };
    glm::vec3 offsetRotation = { 0.0f, 0.0f, 0.0f };
};

struct ColliderComponent
{
    enum class Mode
    {
        SOLID_COLLIDER,
        TRIGGER_SENSOR,
        LEVEL_EDITOR        //Editor selection
    };

    int mode = 0;

    //Array of shapes
    std::vector<ColliderData> shapes;

    //AABB
    glm::vec3 minBounds = { -0.5f, -0.5f, -0.5f };
    glm::vec3 maxBounds = { 0.5f, 0.5f, 0.5f };

    ColliderComponent()
    {
        shapes.emplace_back(); //Add one box by default
    }

    Mode GetMode() const { return static_cast<Mode>(mode); }
};

#endif // COLLIDER_COMPONENT