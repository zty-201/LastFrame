/**
 * @file Light.h
 * @brief Provides utility functions for managing light components in a 3D graphics application.
 *
 * This file defines a collection of inline functions within the `Light` namespace for manipulating
 * light components. It includes functionality for setting light uniforms in shader programs,
 * handling cursor movement to adjust light positions, and handling scroll events to zoom the light.
 *
 * The `LightComponent` structure represents a point light source with position and intensity
 * properties, while the `ShaderProgram` class is used to manage shader operations.
 *
 * @author Zhang TongYan
 * @date Sept, 2025
 */

#ifndef LIGHT_H
#define LIGHT_H
#include "ShaderProgram.h"
//
// The Light struct represents a point light source in 3D space with ambient, 
// diffuse, and specular intensities.
//
namespace Light
{

    inline void onCursor(LightComponent& light, double xoffset, double yoffset)
    {
        const float PI05 = glm::pi<float>() / 2.0f;
        const float r = glm::length(light.position); // Calculate radial distance

        // If radius is zero, we can't calculate angles, so just return.
        if (r < 0.001f) return;

        float alpha = glm::asin(light.position.y / r); // Calculate elevation angle
        float betta = std::atan2f(light.position.x, light.position.z); // Calculate azimuth angle

        // Adjust angles based on cursor offsets
        alpha += yoffset > 0.0 ? -0.05f : 0.05f;
        betta += xoffset < 0.0 ? -0.05f : 0.05f;

        // Clamp alpha to avoid flipping at the poles
        alpha = glm::clamp(alpha, -PI05 + 0.01f, PI05 - 0.01f);

        // Recalculate position based on new angles
        light.position.x = r * glm::cos(alpha) * glm::sin(betta);
        light.position.y = r * glm::sin(alpha);
        light.position.z = r * glm::cos(alpha) * glm::cos(betta);
    }

    /**
     * @brief Handles scroll events to zoom the light in/out by adjusting its radial distance.
     * @param light The LightComponent to modify.
     * @param yoffset The change in the scroll wheel's Y position.
     */
    inline void onScroll(LightComponent& light, double yoffset)
    {
        float r = glm::length(light.position);

        // Adjust radial distance based on scroll direction
        r += yoffset > 0.0 ? -1.0f : 1.0f;
        if (r < 1.0f) r = 1.0f; // Clamp minimum radius

        // Recalculate position by normalizing the old position and scaling by the new radius
        light.position = glm::normalize(light.position) * r;
    }
}

#endif