#ifndef LIGHT_H
#define LIGHT_H

#include "ShaderProgram.h"



//
    // The Light struct represents a point light source in 3D space with ambient, 
    // diffuse, and specular intensities.
    //
struct Light
{
    glm::vec3 position;       // The position of the light in 3D space
    std::vector<glm::vec3> L; // Intensity values for ambient, diffuse, and specular components

    Light(
        glm::vec3 position = { 0.0f, 5.0f, 0.0f }, // Default position is above the origin
        std::initializer_list<glm::vec3> L =      // Default light intensities
        {
            { 0.4f, 0.4f, 0.4f },  // Ambient light intensity (La)
            { 1.0f, 1.0f, 1.0f },  // Diffuse light intensity (Ld)
            { 1.0f, 1.0f, 1.0f }   // Specular light intensity (Ls)
        }
    )
        :
        position{ position },
        L{ L }
    {
    }

    // Sets the light's uniform variables in the given shader program.
    void setUniforms(std::shared_ptr<ShaderProgram> pProgram, int i = 0) const
    {

        std::stringstream s;

        // Set the position of the light
        s << "light[" << i << "].position";
        pProgram->setUniform(s.str(), position);

        // Set ambient light intensity
        if (L.size() > 0)
        {
            s.str("");
            s << "light[" << i << "].L";
            pProgram->setUniform(s.str(), L[0]);

            s.str("");
            s << "light[" << i << "].La";
            pProgram->setUniform(s.str(), L[0]);
        }

        // Set diffuse light intensity
        if (L.size() > 1)
        {
            s.str("");
            s << "light[" << i << "].Ld";
            pProgram->setUniform(s.str(), L[1]);
        }

        // Set specular light intensity
        if (L.size() > 2)
        {
            s.str("");
            s << "light[" << i << "].Ls";
            pProgram->setUniform(s.str(), L[2]);
        }

    }

    // Handles cursor movement events to adjust the light's position.
    void onCursor(double xoffset, double yoffset, std::shared_ptr<ShaderProgram> pProgram = nullptr, int i = 0)
    {
        const float PI05 = glm::pi<float>() / 2.0f;
        const float r = glm::sqrt(position.x * position.x +
            position.y * position.y + position.z * position.z); // Calculate radial distance
        float alpha = glm::asin(position.y / r); // Calculate elevation angle
        float betta = std::atan2f(position.x, position.z); // Calculate azimuth angle

        // Adjust angles based on cursor offsets
        alpha += yoffset > 0.0 ? -0.05f : 0.05f;
        betta += xoffset < 0.0 ? -0.05f : 0.05f;

        // Clamp alpha to avoid flipping at the poles
        alpha = glm::clamp(alpha, -PI05 + 0.01f, PI05 - 0.01f);

        // Recalculate position based on new angles
        position.x = r * glm::cos(alpha) * glm::sin(betta);
        position.y = r * glm::sin(alpha);
        position.z = r * glm::cos(alpha) * glm::cos(betta);

        // Update uniforms in the shader if provided
        if (pProgram)
            setUniforms(pProgram, i);
    }

    // Handles scroll events to zoom the light in/out by adjusting its radial distance.
    void onScroll(double xoffset, double yoffset, std::shared_ptr<ShaderProgram> pProgram = nullptr, int i = 0)
    {
        UNREFERENCED_PARAMETER(xoffset);
        float r = glm::sqrt(position.x * position.x +
            position.y * position.y +
            position.z * position.z); // Calculate radial distance
        const float alpha = glm::asin(position.y / r); // Elevation angle
        const float betta = std::atan2f(position.x, position.z); // Azimuth angle

        // Adjust radial distance based on scroll direction
        r += yoffset > 0.0 ? -1.0f : 1.0f;
        if (r < 1.0f) r = 1.0f; // Clamp minimum radius to avoid collapsing the position

        // Recalculate position based on the new radial distance
        position.x = r * glm::cos(alpha) * glm::sin(betta);
        position.y = r * glm::sin(alpha);
        position.z = r * glm::cos(alpha) * glm::cos(betta);

        // Update uniforms in the shader if provided
        if (pProgram)
            setUniforms(pProgram, i);
    }
};

#endif