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
    std::string name;

    xresource::full_guid albedoMap{};
    xresource::full_guid normalMap{};
    xresource::full_guid metallicMap{};
    xresource::full_guid roughnessMap{};
    xresource::full_guid ambientOcclusionMap{};

    glm::vec3 albedoFactor = { 1.0f, 1.0f, 1.0f };
    float metallicFactor = 1.0f;
    float roughnessFactor = 1.0f;
};

inline bool operator==(const MaterialComponent& a, const MaterialComponent& b)
{
    return  a.roughnessFactor == b.roughnessFactor
        && a.metallicFactor == b.metallicFactor
        && a.albedoFactor == b.albedoFactor
        && a.albedoMap == b.albedoMap
        && a.normalMap == b.normalMap
        && a.metallicMap == b.metallicMap
        && a.roughnessMap == b.roughnessMap
        && a.ambientOcclusionMap == b.ambientOcclusionMap;
}

inline bool operator!=(const MaterialComponent& a, const MaterialComponent& b)
{
    return !(a == b);
}

// --- Other Scene Entity Components ---
#endif
