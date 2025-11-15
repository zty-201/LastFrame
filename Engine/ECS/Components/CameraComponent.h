/**
 * @file CameraComponent.h
 * @brief Defines the CameraComponent structure for managing camera properties in a 3D scene.
 *
 * This file contains the `CameraComponent` structure, which represents a camera in a 3D
 * graphics application. It includes properties for camera type, position, target, field
 * of view, and clipping planes, as well as matrices for view and projection transformations.
 *
 * Key functionalities:
 * - Supports two camera types: ORBITING and WALKING.
 * - Stores camera position, target, and field of view.
 * - Includes matrices for view and projection transformations.
 *
 * @date Sept 2025
 */
#ifndef CAM_COMPONENT
#define CAM_COMPONENT

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct CameraComponent
{
    // Nested enum so Camera.h can use CameraComponent::CameraType
    enum CameraType { ORBITING = 0, WALKING = 1 };

    // --- Mode / identity ---
    CameraType camType{ WALKING };
    bool       is_active{ true };   // optional scene toggle

    // --- Frustum ---
    float fieldOfView{ 75.0f };     // degrees
    float nearPlane{ 0.1f };
    float farPlane{ 100.0f };

    // --- Pose helpers (Camera.h manipulates these) ---
    glm::vec3 position{ 0.0f, 1.8f, 0.0f };
    glm::vec3 target{ 0.0f, 1.8f, -1.0f };

    // --- Cached matrices (Camera.h writes/reads these) ---
    glm::mat4 view_vector{ 1.0f };  // keep exact name for compatibility
    glm::mat4 proj_matrix{ 1.0f };

    // --- Input state used by Camera.h handlers ---
    double oldx{ 0.0 };
    double oldy{ 0.0 };
};

#endif