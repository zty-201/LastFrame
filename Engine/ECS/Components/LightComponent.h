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
    enum Type
    {
        DIRECTIONAL = 0,
        POINT = 1,
        SPOT = 2
	};
    Type type; // false = directional, true = point/spot
    glm::vec3 position;    // For point/spot
    glm::vec3 direction;   // For directional/spot
    glm::vec3 color{ 1.0f, 1.0f, 1.0f };
    float intensity{ 1.0f };
    float radius{ 1.0f };          // For point/spot
    float constant{ 1.0f }, linear{ 0.09f }, quadratic{ 0.032f }; // For point/spot
};

#endif