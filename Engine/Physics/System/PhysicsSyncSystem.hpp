#pragma once
#include <entt/entt.hpp>
#include "TransformComponent.h"
#include "PhysicsComponent.h"
#include "PhysicsWorld.hpp"

class PhysicsSyncSystem
{
public:
    static void Update();
};