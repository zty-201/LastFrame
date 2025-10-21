/**
 * @file MaterialComponent.h
 * @brief Defines the MaterialComponent structure for representing material properties in a 3D scene.
 *
 * This file contains the `MaterialComponent` structure, which represents the material properties
 * of an object in a 3D graphics application. It includes properties for roughness, metallicity,
 * color, ambient, diffuse, and specular components, as well as shininess and effect parameters.
 *
 * Key functionalities:
 * - Stores material properties such as roughness, metallicity, and color.
 * - Includes predefined materials for PBR, discard, cartoon, and checkerboard effects.
 * - Provides comparison operators for equality and inequality.
 *
 * @date Sept 2025
 */
#ifndef MATERIAL_COMPONENT
#define MATERIAL_COMPONENT
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
	std::string materialName;
};

inline bool operator==(const MaterialComponent& a, const MaterialComponent& b)
{
    return  a.rough == b.rough
        && a.metal == b.metal
        && a.shininess == b.shininess
        && a.effect == b.effect
        && a.color == b.color
        && a.Ka == b.Ka
        && a.Kd == b.Kd
        && a.Ks == b.Ks;
}

inline bool operator!=(const MaterialComponent& a, const MaterialComponent& b)
{
    return !(a == b);
}


// Define some default materials using the new structure (to be deleted later)
const MaterialComponent MATERIAL_PBR_PHONG({ 0.8f, 0.2f, {1.0f, 0.8f, 0.2f}, {0.1f, 0.1f, 0.1f}, {0.7f, 0.7f, 0.7f}, {1.0f, 1.0f, 1.0f}, 128.0f, 0.0f, "PBR"});
const MaterialComponent MATERIAL_DISCARD({ 0.5f, 0.0f, {0.2f, 0.8f, 0.2f}, {0.1f, 0.1f, 0.1f}, {0.7f, 0.7f, 0.7f}, {1.0f, 1.0f, 1.0f}, 32.0f, 1.0f, "Discard"});
const MaterialComponent MATERIAL_CARTOON({ 0.5f, 0.0f, {0.8f, 0.2f, 0.8f}, {0.1f, 0.1f, 0.1f}, {0.7f, 0.7f, 0.7f}, {1.0f, 1.0f, 1.0f}, 32.0f, 2.0f, "Cartoon"});
const MaterialComponent MATERIAL_CHECKERBOARD({ 0.9f, 0.0f, {1.0f, 1.0f, 1.0f}, {0.1f, 0.1f, 0.1f}, {0.7f, 0.7f, 0.7f}, {1.0f, 1.0f, 1.0f}, 32.0f, 3.0f, "Checkerboard"});

// --- Other Scene Entity Components ---
#endif
