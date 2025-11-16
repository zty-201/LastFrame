#pragma once

// Adjust these includes to match your engine's include layout.
#include "CameraComponent.h"        // Your existing camera component
#include "TransformComponent.h"     // <-- Adjust path/name to your Transform component
#include <glm/glm.hpp>

// entt forward declarations (keeps compile times small)
#include <entt/entity/fwd.hpp>

/// Input snapshot for the editor camera in a single frame.
/// Filled by Screen.cpp (from ImGui / input system) and passed into the controller.
struct EditorCameraInput
{
    // Keyboard movement
    bool keyW = false;
    bool keyA = false;
    bool keyS = false;
    bool keyD = false;
    bool keyQ = false;   // up
    bool keyE = false;   // down

    bool keyUp = false;
    bool keyDown = false;
    bool keyLeft = false;
    bool keyRight = false;

    // Focus / frame selected
    bool keyFocus = false;   // e.g. F key

    // Mouse buttons
    bool rightMouseDown = false;   // orbit / look
    bool middleMouseDown = false;   // pan / drag

    // Mouse movement
    float mouseDeltaX = 0.0f;
    float mouseDeltaY = 0.0f;

    // Scroll wheel (positive = scroll up)
    float mouseScroll = 0.0f;
};

/// Controller responsible ONLY for editor camera behaviour:
/// - orbiting around a target/selected object
/// - panning / dragging
/// - zoom / dolly
/// - keyboard movement
///
/// Screen.cpp should:
///   1. Build an EditorCameraInput from ImGuiIO
///   2. Find the active CameraComponent
///   3. Call EditorCameraController::Update(...)
class EditorCameraController
{
public:
    /// Main entry point. Call this once per frame from the level editor screen.
    ///
    /// \param cam       The active editor camera component.
    /// \param input     The current frame's input snapshot.
    /// \param registry  ECS registry (used for focusing on selected entities).
    /// \param selected  Currently selected entity (entt::null if none).
    void Update(CameraComponent& cam,
        const EditorCameraInput& input,
        entt::registry& registry,
        entt::entity selected);

    // Tuning knobs (you can tweak from Screen if needed).
    float moveSpeed = 0.5f;   ///< WASD/QE movement speed
    float orbitSpeed = 0.3f;   ///< Orbit sensitivity
    float panSpeed = 0.01f;  ///< Pan sensitivity
    float zoomSpeed = 0.5f;   ///< Scroll dolly speed

private:
    void HandleKeyboardMove(CameraComponent& cam,
        const EditorCameraInput& in);

    void HandleOrbitAndZoom(CameraComponent& cam,
        const EditorCameraInput& in);

    void HandlePan(CameraComponent& cam,
        const EditorCameraInput& in);

    void FocusOnSelected(CameraComponent& cam,
        entt::registry& registry,
        entt::entity selected);
};