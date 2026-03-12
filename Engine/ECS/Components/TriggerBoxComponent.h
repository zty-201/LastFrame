#pragma once
#include <cstdint>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

// Simple trigger volumes that fire overlap events.
// Does NOT require a PhysicsComponent. Build system will create a STATIC sensor body.
//
// Notes:
// - Angles are in degrees to match typical editor UX.
// - Local transform is relative to the entity's TransformComponent.
// - Only one trigger per component. (Multiple triggers per entity = add multiple components or child entities.)
namespace EngineFramework
{
    class TriggerSystem; //Forward declarations
}
enum class TriggerShapeType : uint8_t {
    Box = 0,
    Sphere,
    Capsule
};

struct TriggerBoxComponent
{
    // --- authoring ---
    bool                enabled = true;                    // turn on/off without removing
    bool                highlight = false;

    TriggerShapeType    type = TriggerShapeType::Box;   // which shape to use

    // Box parameters (used when type == Box)
    glm::vec3           halfExtents = { 0.5f, 0.5f, 0.5f };      // half sizes (x,y,z)

    // Sphere parameters (used when type == Sphere)
    float               radius = 0.5f;                    // sphere radius

    // Capsule parameters (used when type == Capsule)
    float               capHalfHeight = 0.5f;                    // half height of cylinder part
    float               capRadius = 0.5f;                    // capsule radius

    // Local offset/orientation relative to the entity transform
    glm::vec3           localPosition = { 0.0f, 0.0f, 0.0f };
    glm::vec3           localEulerDeg = { 0.0f, 0.0f, 0.0f };      // pitch(y), yaw(z), roll(x) as you prefer in your math utils

    // Optional: visual debug (ignored by runtime)
    glm::vec3           debugColor = { 0.1f, 0.8f, 0.2f };      // for gizmos

    // runtime
    std::unordered_set<entt::entity> currentInside;
    bool is_collided_this_frame = false;

    // --- runtime (filled by TriggerBuildSystem) ---
    static constexpr uint32_t InvalidId = 0xFFFFFFFFu;
    uint32_t            bodyId = InvalidId;              // engine/Jolt id (opaque here)
    bool                isRegistered = false;                   // created and added to the world this session

private:
    friend class EngineFramework::TriggerSystem;
    // Temp values used to indicate any changes in current triggerbox, i.e. need recalculation //
    glm::vec3 old_localPosition{};
    glm::vec3 old_localEulerDeg{};
    glm::vec3 old_halfExtents{};
    float old_radius{};
    float old_capHalfHeight{};
    float old_capRadius{};
    glm::vec3 old_parent_pos{};
    glm::vec3 old_parent_rotation{};
    glm::vec3 old_parent_scale{};

    //Temp value of AABB, stored to prevent unnecessary recalculation.
    glm::vec3 min{};
    glm::vec3 max{};
};
