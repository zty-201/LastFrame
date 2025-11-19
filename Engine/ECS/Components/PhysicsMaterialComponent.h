#pragma once

#ifndef PHYSICSMATERIALCOMPONENT
#define PHYSICSMATERIALCOMPONENT

struct PhysicsMaterialComponent
{
    float friction = 0.5f;
    float restitution = 0.0f;
    float density = 1000.0f;

    bool isStatic = false; // Static vs Dynamic/Kinematic
    bool _prevIsStatic = false;

    glm::vec3 _prevScale = glm::vec3(1.0f); //used to detect scale changes

    int  layer = 1;        // Collision filtering layer
};

#endif