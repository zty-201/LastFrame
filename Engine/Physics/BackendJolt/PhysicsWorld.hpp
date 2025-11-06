#pragma once
#include <Jolt/Jolt.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <unordered_map>
#include <entt/entity/entity.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

class PhysicsWorld
{
public:
    static PhysicsWorld& Get(); // Singleton access

    void Initialize();
    void Shutdown();

    void StepSimulation(float dt);

    // Body management
    void CreateBoxBody(entt::entity entity, const glm::vec3& position, const glm::vec3& size, bool isStatic);
    void CreateSphereBody(entt::entity entity, const glm::vec3& position, float radius, bool isStatic);
    void CreateCapsuleBody(entt::entity entity, const glm::vec3& position, float height, float radius, bool isStatic);
    void RemoveBody(entt::entity entity);

    // Body state access
    bool HasBody(entt::entity entity) const;
    glm::vec3 GetBodyPosition(entt::entity entity) const;
    glm::quat GetBodyRotation(entt::entity entity) const;
    glm::vec3 GetBodyVelocity(entt::entity entity) const;

    void SetLinearVelocity(entt::entity entity, const glm::vec3& velocity);
    void AddForce(entt::entity entity, const glm::vec3& force);

private:
    PhysicsWorld() = default;
    ~PhysicsWorld() = default;

    JPH::PhysicsSystem* m_physicsSystem = nullptr;
    JPH::JobSystemThreadPool* m_jobSystem = nullptr;

    std::unordered_map<entt::entity, JPH::BodyID> m_entityToBody;
    std::unordered_map<JPH::BodyID, entt::entity> m_bodyToEntity;

    float m_accumulator = 0.0f;
    const float m_fixedDt = 1.0f / 60.0f;
};