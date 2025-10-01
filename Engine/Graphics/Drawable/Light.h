#ifndef LIGHT_H
#define LIGHT_H
#include "ShaderProgram.h"
//
// The Light struct represents a point light source in 3D space with ambient, 
// diffuse, and specular intensities.
//
namespace Light
{

    // Sets the light's uniform variables in the given shader program.
    inline void setUniforms(LightComponent& light, std::shared_ptr<ShaderProgram> pProgram, int i = 0)
    {

        std::stringstream s;

        // Set the position of the light
        s << "light[" << i << "].position";
        pProgram->setUniform(s.str(), light.position);

        // Set ambient light intensity
        if (light.L.size() > 0)
        {
            s.str("");
            s << "light[" << i << "].L";
            pProgram->setUniform(s.str(), light.L[0]);

            s.str("");
            s << "light[" << i << "].La";
            pProgram->setUniform(s.str(), light.L[0]);
        }

        // Set diffuse light intensity
        if (light.L.size() > 1)
        {
            s.str("");
            s << "light[" << i << "].Ld";
            pProgram->setUniform(s.str(), light.L[1]);
        }

        // Set specular light intensity
        if (light.L.size() > 2)
        {
            s.str("");
            s << "light[" << i << "].Ls";
            pProgram->setUniform(s.str(), light.L[2]);
        }

    }

    // Handles cursor movement events to adjust the light's position.
    inline void onCursor(LightComponent& light, double xoffset, double yoffset, std::shared_ptr<ShaderProgram> pProgram = nullptr, int i = 0)
    {
        const float PI05 = glm::pi<float>() / 2.0f;
        const float r = glm::sqrt(light.position.x * light.position.x +
            light.position.y * light.position.y + light.position.z * light.position.z); // Calculate radial distance
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

        // Update uniforms in the shader if provided
        if (pProgram)
            Light::setUniforms(light, pProgram, i);
    }

    // Handles scroll events to zoom the light in/out by adjusting its radial distance.
    inline void onScroll(LightComponent& light, double xoffset, double yoffset, std::shared_ptr<ShaderProgram> pProgram = nullptr, int i = 0)
    {
        UNREFERENCED_PARAMETER(xoffset);
        float r = glm::sqrt(light.position.x * light.position.x +
            light.position.y * light.position.y +
            light.position.z * light.position.z); // Calculate radial distance
        const float alpha = glm::asin(light.position.y / r); // Elevation angle
        const float betta = std::atan2f(light.position.x, light.position.z); // Azimuth angle

        // Adjust radial distance based on scroll direction
        r += yoffset > 0.0 ? -1.0f : 1.0f;
        if (r < 1.0f) r = 1.0f; // Clamp minimum radius to avoid collapsing the position

        // Recalculate position based on the new radial distance
        light.position.x = r * glm::cos(alpha) * glm::sin(betta);
        light.position.y = r * glm::sin(alpha);
        light.position.z = r * glm::cos(alpha) * glm::cos(betta);

        // Update uniforms in the shader if provided
        if (pProgram)
            setUniforms(light, pProgram, i);
    }
}

#endif