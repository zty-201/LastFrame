#ifndef MATERIAL_H
#define MATERIAL_H

#include "ShaderProgram.h"
#include <glm/glm.hpp>

struct Material
{
    float rough;
    float metal;
    glm::vec3 color;
    glm::vec3 Ka;
    glm::vec3 Kd;
    glm::vec3 Ks;
    float shininess;
    float effect; // *** ADD THIS LINE ***

    // Default constructor
    Material(float rough = 0.5f, float metal = 0.0f, glm::vec3 color = { 1.0f, 1.0f, 1.0f },
        glm::vec3 Ka = { 0.1f, 0.1f, 0.1f }, glm::vec3 Kd = { 0.7f, 0.7f, 0.7f }, glm::vec3 Ks = { 1.0f, 1.0f, 1.0f },
        float shininess = 32.0f, float effect = 0.0f) // Add effect to constructor
        : rough(rough), metal(metal), color(color), Ka(Ka), Kd(Kd), Ks(Ks), shininess(shininess), effect(effect) {
    }

    void setUniforms(ShaderProgram* shader) const
    {
        shader->setUniform("material.rough", rough);
        shader->setUniform("material.metal", metal);
        shader->setUniform("material.color", color);
        shader->setUniform("material.Ka", Ka);
        shader->setUniform("material.Kd", Kd);
        shader->setUniform("material.Ks", Ks);
        shader->setUniform("material.shininess", shininess);
        shader->setUniform("material.effect", effect); // *** ADD THIS LINE ***
    }
};

// Define some default materials using the new structure
const Material MATERIAL_PBR_PHONG({ 0.8f, 0.2f, {1.0f, 0.8f, 0.2f}, {0.1f, 0.1f, 0.1f}, {0.7f, 0.7f, 0.7f}, {1.0f, 1.0f, 1.0f}, 128.0f, 0.0f });
const Material MATERIAL_DISCARD({ 0.5f, 0.0f, {0.2f, 0.8f, 0.2f}, {0.1f, 0.1f, 0.1f}, {0.7f, 0.7f, 0.7f}, {1.0f, 1.0f, 1.0f}, 32.0f, 1.0f });
const Material MATERIAL_CARTOON({ 0.5f, 0.0f, {0.8f, 0.2f, 0.8f}, {0.1f, 0.1f, 0.1f}, {0.7f, 0.7f, 0.7f}, {1.0f, 1.0f, 1.0f}, 32.0f, 2.0f });
const Material MATERIAL_CHECKERBOARD({ 0.9f, 0.0f, {1.0f, 1.0f, 1.0f}, {0.1f, 0.1f, 0.1f}, {0.7f, 0.7f, 0.7f}, {1.0f, 1.0f, 1.0f}, 32.0f, 3.0f });

#endif // MATERIAL_H