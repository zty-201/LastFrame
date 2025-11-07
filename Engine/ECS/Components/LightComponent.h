/**
 * @file LightComponent.h
 * @brief Defines the LightComponent structure for representing light properties in a 3D scene.
 *
 * This file contains the `LightComponent` structure, which represents a light source in a
 * 3D graphics application. It includes properties for the light's position and intensity
 * values for ambient, diffuse, and specular components.
 *
 * Key functionalities:
 * - Stores the position of the light in 3D space.
 * - Holds intensity values for ambient, diffuse, and specular lighting.
 *
 * @date Sept 2025
 */
#ifndef LIGHT_COMPONENT
#define LIGHT_COMPONENT

struct LightComponent
{
    glm::vec3 position{};
    glm::vec3 color{ 1.0f, 1.0f, 1.0f }; // The actual color of the light (e.g., white)
    float intensity{ 1.0f };
};

#endif