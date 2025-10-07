/**
 * @file NormalMapComponent.h
 * @brief Defines the NormalMapComponent structure for associating entities with normal maps.
 *
 * This file contains the `NormalMapComponent` structure, which represents a reference
 * to a normal map resource in a 3D graphics application. Normal maps are used to add
 * surface detail and enhance the appearance of lighting on 3D objects.
 *
 * Key functionalities:
 * - Stores a GUID (Globally Unique Identifier) for the associated normal map resource.
 *
 * @date Sept 2025
 */
#ifndef NORMAL_MAP_COMPONENT
#define NORMAL_MAP_COMPONENT
struct NormalMapComponent
{
    xresource::full_guid normalmap_guid; // Normal map only
};

#endif