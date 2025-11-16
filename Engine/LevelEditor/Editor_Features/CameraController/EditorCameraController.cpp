#include "EditorCameraController.hpp"
#include "Camera.h"

#include <entt/entity/registry.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

// No experimental GLM headers, no glm::length2 used.

void EditorCameraController::Update(CameraComponent& cam,
    const EditorCameraInput& in,
    entt::registry& ecsRegistry,
    entt::entity selected)
{
    // 1. Basic keyboard movement (WASD, Q/E, arrows)
    HandleKeyboardMove(cam, in);

    // 2. Mouse-based orbit + scroll-based zoom/dolly
    HandleOrbitAndZoom(cam, in);

    // 3. Middle-mouse pan
    HandlePan(cam, in);

    // 4. Frame / focus selected object (e.g., F key)
    if (in.keyFocus)
    {
        FocusOnSelected(cam, ecsRegistry, selected);
    }
}

// -----------------------------------------------------------------------------
//  Keyboard movement
// -----------------------------------------------------------------------------
void EditorCameraController::HandleKeyboardMove(CameraComponent& cam,
    const EditorCameraInput& in)
{
    glm::vec3 delta(0.0f);

    if (in.keyW)       delta.z += moveSpeed;
    if (in.keyS)       delta.z -= moveSpeed;
    if (in.keyQ || in.keyUp)    delta.y += moveSpeed;
    if (in.keyE || in.keyDown)  delta.y -= moveSpeed;
    if (in.keyA || in.keyLeft)  delta.x -= moveSpeed;
    if (in.keyD || in.keyRight) delta.x += moveSpeed;

    if (delta.x == 0.0f && delta.y == 0.0f && delta.z == 0.0f)
        return;

    // Use your existing helper so behaviour matches existing engine logic.
    Camera::Move_Cam_and_Target(cam, delta);
}

// -----------------------------------------------------------------------------
//  Orbit around target + scroll zoom/dolly
// -----------------------------------------------------------------------------
void EditorCameraController::HandleOrbitAndZoom(CameraComponent& cam,
    const EditorCameraInput& in)
{
    // RMB drag -> orbit around current target (in ORBITING mode)
    if (in.rightMouseDown && cam.camType == CameraComponent::ORBITING)
    {
        // Vector from target to camera
        glm::vec3 offset = cam.position - cam.target;
        float radius = glm::length(offset);

        if (radius < 0.001f)
            radius = 0.001f;

        // Convert to spherical-ish coordinates
        float yaw = std::atan2(offset.x, offset.z);
        float pitch = std::atan2(offset.y,
            glm::length(glm::vec2(offset.x, offset.z)));

        // Mouse X -> yaw, mouse Y -> pitch
        constexpr float mouseScale = 0.01f;
        yaw -= in.mouseDeltaX * orbitSpeed * mouseScale;
        pitch -= in.mouseDeltaY * orbitSpeed * mouseScale;

        // Clamp pitch to avoid flipping
        const float pitchLimit = glm::radians(89.0f);
        pitch = glm::clamp(pitch, -pitchLimit, pitchLimit);

        // Convert back to Cartesian
        glm::vec3 newOffset;
        newOffset.x = radius * std::sin(yaw) * std::cos(pitch);
        newOffset.z = radius * std::cos(yaw) * std::cos(pitch);
        newOffset.y = radius * std::sin(pitch);

        cam.position = cam.target + newOffset;
    }
    else if (in.rightMouseDown && cam.camType == CameraComponent::WALKING)
    {
        // FPS-style look in walking mode: use your existing helper
        Camera::onCursor(cam, in.mouseDeltaX, in.mouseDeltaY);
    }

    // Scroll wheel -> dolly towards/away from target
    if (in.mouseScroll != 0.0f)
    {
        glm::vec3 dir = cam.target - cam.position;
        float distance = glm::length(dir);
        if (distance > 0.000001f)
        {
            dir /= distance;

            float distanceFactor = distance * 0.1f; // faster when far away
            float step = in.mouseScroll * zoomSpeed * distanceFactor;

            cam.position += dir * step;
        }
    }
    /*if (in.mouseScroll != 0.0f)
    {
        glm::vec3 dir = cam.target - cam.position;
        float len = glm::length(dir);
        if (len > 0.000001f)
        {
            dir /= len;
            cam.position += dir * (in.mouseScroll * zoomSpeed);
        }
    }*/
}

// -----------------------------------------------------------------------------
//  Pan / drag camera (MMB)
// -----------------------------------------------------------------------------
void EditorCameraController::HandlePan(CameraComponent& cam,
    const EditorCameraInput& in)
{
    if (!in.middleMouseDown)
        return;

    glm::vec3 forward = cam.target - cam.position;
    float distance = glm::length(forward);
    if (distance < 0.000001f)
        return;

    forward /= distance;

    const glm::vec3 worldUp(0.0f, 1.0f, 0.0f);
    glm::vec3 right = glm::normalize(glm::cross(forward, worldUp));
    glm::vec3 up = glm::normalize(glm::cross(right, forward));

    // Scale pan speed by distance so it feels consistent
    float distanceFactor = distance * 0.001f;  // tweak this constant
    float dx = -in.mouseDeltaX * panSpeed * distanceFactor;
    float dy = in.mouseDeltaY * panSpeed * distanceFactor;

    glm::vec3 delta = dx * right + dy * up;

    cam.position += delta;
    cam.target += delta;
}

// -----------------------------------------------------------------------------
//  Focus / frame selected entity
// -----------------------------------------------------------------------------
void EditorCameraController::FocusOnSelected(CameraComponent& cam,
    entt::registry& ecsRegistry,
    entt::entity selected)
{
    if (selected == entt::null)
        return;

    if (!ecsRegistry.all_of<TransformComponent>(selected))
        return;

    auto& tf = ecsRegistry.get<TransformComponent>(selected);

    // World position of the object we want to frame
    glm::vec3 objectPos = tf.position;  // adjust if your field name differs

    // ---- Decide how far the camera should be ----

    // Current distance from camera to its target
    glm::vec3 camToTarget = cam.target - cam.position;
    float currentDistance = glm::length(camToTarget);

    // Unity-like behaviour: clamp distance into a useful range.
    // If camera is very far or extremely close, snap to a default distance.
    const float minUsefulDist = 100.0f;   // too close -> uncomfortable
    const float maxUsefulDist = 500.0f;  // too far  -> feels like nothing happened
    const float defaultFocusDist = 200.0f;   // good "frame" distance for most objects

    float focusDistance = currentDistance;
    if (focusDistance < minUsefulDist || focusDistance > maxUsefulDist)
        focusDistance = defaultFocusDist;

    // ---- Choose view direction ----

    glm::vec3 viewDir;

    // Use current view direction if possible
    if (currentDistance > 0.000001f)
        viewDir = glm::normalize(camToTarget);
    else
        viewDir = glm::vec3(0.0f, 0.0f, -1.0f);

    // ---- Move camera to frame the object ----

    cam.target = objectPos;
    cam.position = objectPos - viewDir * focusDistance;

    // Switch to orbiting mode so RMB orbit feels natural.
    cam.camType = CameraComponent::ORBITING;
}
//void EditorCameraController::FocusOnSelected(CameraComponent& cam,
//    entt::registry& ecsRegistry,
//    entt::entity selected)
//{
//    if (selected == entt::null)
//        return;
//
//    if (!ecsRegistry.all_of<TransformComponent>(selected))
//        return;
//
//    auto& tf = ecsRegistry.get<TransformComponent>(selected);
//
//    // Assumes your TransformComponent has a world-space position field.
//    glm::vec3 objectPos = tf.position;  // Adjust if your field name differs.
//
//    // Maintain current distance from camera to target if possible.
//    glm::vec3 camToTarget = cam.target - cam.position;
//    float distance = glm::length(camToTarget);
//
//    if (distance < 0.001f)
//        distance = 5.0f; // fallback distance if camera and target coincide
//
//    glm::vec3 viewDir;
//    float len = glm::length(camToTarget);
//    if (len > 0.000001f)
//        viewDir = camToTarget / len;
//    else
//        viewDir = glm::vec3(0.0f, 0.0f, -1.0f);
//
//    // New pivot is the object's position.
//    cam.target = objectPos;
//    cam.position = objectPos - viewDir * distance;
//
//    // Switch to orbiting mode when focused so RMB orbit feels natural.
//    cam.camType = CameraComponent::ORBITING;
//}