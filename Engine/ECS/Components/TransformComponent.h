#ifndef TRANSFORM_COMPONENT
#define TRANSFORM_COMPONENT

// --- Core Components ---

struct TransformComponent {
    glm::vec3 position = { 0.0f, 0.0f, 0.0f };
    glm::vec3 rotation = { 0.0f, 0.0f, 0.0f }; // Euler angles
    glm::vec3 scale = { 1.0f, 1.0f, 1.0f };
	glm::mat4 modelMatrix = glm::mat4(1.0f);
};


// Tag to mark an entity as being part of the main scene rendering pass
struct SceneRenderPass {};

// Tag to mark an entity as being part of the skybox rendering pass
struct SkyboxRenderPass {};
#endif