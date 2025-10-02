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