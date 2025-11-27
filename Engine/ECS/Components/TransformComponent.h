/**
 * @file TransformComponent.h
 * @brief Defines the TransformComponent structure and rendering pass tags for 3D entities.
 *
 * This file contains the `TransformComponent` structure, which represents the transformation
 * properties of an entity in a 3D graphics application. It includes position, rotation, scale,
 * and a model matrix for rendering. Additionally, it defines tags for marking entities as part
 * of specific rendering passes.
 *
 * Key functionalities:
 * - Stores transformation properties such as position, rotation (Euler angles), and scale.
 * - Includes a model matrix for rendering transformations.
 * - Provides tags for scene rendering (`SceneRenderPass`) and skybox rendering (`SkyboxRenderPass`).
 *
 * @date Sept 2025
 */
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#ifndef TRANSFORM_COMPONENT
#define TRANSFORM_COMPONENT

// --- Core Components ---

struct TransformComponent {
    glm::vec3 position = { 0.0f, 0.0f, 0.0f };
    glm::vec3 rotation = { 0.0f, 0.0f, 0.0f }; // Euler angles
    glm::vec3 scale = { 1.0f, 1.0f, 1.0f };

	glm::mat4 modelMatrix = glm::mat4(1.0f);

    glm::mat4 prevModelMatrix = glm::mat4(1.0f);
};


// Tag to mark an entity as being part of the main scene rendering pass
struct SceneRenderPass {};

// Tag to mark an entity as being part of the skybox rendering pass
struct SkyboxRenderPass {};
#endif