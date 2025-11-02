#pragma once
#include <entt/entt.hpp>
#include "TransformComponent.h"
#include "PhysicsComponent.h"
#include "ColliderComponent.h"
#include "PhysicsWorld.hpp"

class PhysicsBuildSystem
{
public:
    static void Update();
};