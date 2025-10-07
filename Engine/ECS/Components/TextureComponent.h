/**
 * @file TextureComponent.h
 * @brief Defines the TextureComponent structure for associating entities with texture resources.
 *
 * This file contains the `TextureComponent` structure, which represents a reference
 * to a texture resource in a 3D graphics application. Textures are used to enhance
 * the visual appearance of 3D objects by mapping images onto their surfaces.
 *
 * Key functionalities:
 * - Stores a GUID (Globally Unique Identifier) for the associated texture resource.
 *
 * @date Sept 2025
 */
#ifndef TEXTURE_COMPONENT
#define TEXTURE_COMPONENT
struct TextureComponent
    {
	    xresource::full_guid texture_guid; // Full GUID for the texture resource
    };
#endif // !TEXTURE_COMPONENT
