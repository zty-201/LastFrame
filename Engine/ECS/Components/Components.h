#pragma once

// --- Core Components ---

struct TransformComponent {
    glm::vec3 position = { 0.0f, 0.0f, 0.0f };
    glm::vec3 rotation = { 0.0f, 0.0f, 0.0f }; // Euler angles
    glm::vec3 scale = { 1.0f, 1.0f, 1.0f };
};

// --- Renderable Object Components ---

// A handle to a mesh resource (VAO, buffers, etc.)
// This avoids duplicating mesh data for every entity.
struct MeshComponent {

    // Indices defining the connectivity of the mesh. Each group of three indices
    // corresponds to a single triangle.
    std::vector<GLuint> indices;

    // Vertex positions of the mesh. Each set of three consecutive floats represents
    // a single vertex position (x, y, z).
    std::vector<GLfloat> points;

    // Vertex normals used for lighting calculations. Each set of three consecutive
    // floats represents a normal vector (nx, ny, nz).
    std::vector<GLfloat> normals;

    // Texture coordinates for mapping textures to the mesh surface. Each set of
    // two consecutive floats represents a texture coordinate (u, v).
    std::vector<GLfloat> texCoords;

    // Tangent vectors for each vertex. These are used in advanced lighting techniques,
    // such as normal mapping, to compute tangent-space transformations. Each set of
    // three consecutive floats represents a tangent vector (tx, ty, tz).
    std::vector<GLfloat> tangents;

};

struct MaterialComponent
{
    float rough;
    float metal;
    glm::vec3 color;
    glm::vec3 Ka;
    glm::vec3 Kd;
    glm::vec3 Ks;
    float shininess;
    float effect; 

    // Default constructor
    MaterialComponent(float rough = 0.5f, float metal = 0.0f, glm::vec3 color = { 1.0f, 1.0f, 1.0f },
        glm::vec3 Ka = { 0.1f, 0.1f, 0.1f }, glm::vec3 Kd = { 0.7f, 0.7f, 0.7f }, glm::vec3 Ks = { 1.0f, 1.0f, 1.0f },
        float shininess = 32.0f, float effect = 0.0f) // Add effect to constructor
        : rough(rough), metal(metal), color(color), Ka(Ka), Kd(Kd), Ks(Ks), shininess(shininess), effect(effect) {
    }

    
};

// Define some default materials using the new structure (to be deleted later)
const MaterialComponent MATERIAL_PBR_PHONG({ 0.8f, 0.2f, {1.0f, 0.8f, 0.2f}, {0.1f, 0.1f, 0.1f}, {0.7f, 0.7f, 0.7f}, {1.0f, 1.0f, 1.0f}, 128.0f, 0.0f });
const MaterialComponent MATERIAL_DISCARD({ 0.5f, 0.0f, {0.2f, 0.8f, 0.2f}, {0.1f, 0.1f, 0.1f}, {0.7f, 0.7f, 0.7f}, {1.0f, 1.0f, 1.0f}, 32.0f, 1.0f });
const MaterialComponent MATERIAL_CARTOON({ 0.5f, 0.0f, {0.8f, 0.2f, 0.8f}, {0.1f, 0.1f, 0.1f}, {0.7f, 0.7f, 0.7f}, {1.0f, 1.0f, 1.0f}, 32.0f, 2.0f });
const MaterialComponent MATERIAL_CHECKERBOARD({ 0.9f, 0.0f, {1.0f, 1.0f, 1.0f}, {0.1f, 0.1f, 0.1f}, {0.7f, 0.7f, 0.7f}, {1.0f, 1.0f, 1.0f}, 32.0f, 3.0f });

// Tag to mark an entity as being part of the main scene rendering pass
struct SceneRenderPass {};

// Tag to mark an entity as being part of the skybox rendering pass
struct SkyboxRenderPass {};

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

// Your existing Camera/Light structs are already data-oriented and can be used as components.
//using CameraComponent = Camera;
//using LightComponent = Light;

// Component to tag the primary/active camera for a scene
//struct ActiveCamera {};
