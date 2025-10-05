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

struct CameraComponent
{
    enum CameraType { ORBITING, WALKING };

    CameraType camType; // Stores the type of camera (ORBITING or WALKING)
    glm::vec3 position; // Current position of the camera in 3D space
    glm::vec3 target;   // Target point that the camera is looking at
    float fieldOfView;  // Field of view angle in degrees
    float nearPlane;         // Near clipping plane
    float farPlane;          // Far clipping plane

    glm::mat4 view_vector;
    glm::mat4 proj_matrix;

    double oldx; // Previous x-coordinate for cursor handling
    double oldy; // Previous y-coordinate for cursor handling
};
// --- Other Scene Entity Components ---

// Component to tag the primary/active camera for a scene
//struct ActiveCamera {};
#endif