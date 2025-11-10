#pragma once
#include <glm/glm.hpp>

#ifndef PHYSICS_COMPONENT
#define PHYSICS_COMPONENT

struct PhysicsComponent
{
    bool isStatic = false;          // true = doesn't move (floor, wall)

    glm::vec3 velocity{ 0.0f, 0.0f, 0.0f };     // current linear velocity
    glm::vec3 acceleration{ 0.0f, 0.0f, 0.0f }; // per-frame accel (input, gravity override, etc.)

    float friction = 0.1f;          // simple drag (0 = no slowdown, higher = more slowdown)
    float mass = 1.0f;              // can be passed to Jolt later if needed

    bool  useGravity = true;
    float gravityFactor = 1.0f;

};
#endif // PHYSICS_COMPONENT