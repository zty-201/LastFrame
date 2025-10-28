/**
 * @file MeshComponent.h
 * @brief Defines the MeshComponent structure for associating entities with mesh resources.
 *
 * This file contains the `MeshComponent` structure, which represents a handle to a mesh
 * resource in a 3D graphics application. It avoids duplicating mesh data for every entity
 * by referencing shared mesh resources.
 *
 * Key functionalities:
 * - Stores the name of the mesh resource (e.g., "Sphere", "Cube", or external files like "tree.obj").
 * - Optionally caches the VAO (Vertex Array Object) and index count after loading the mesh.
 *
 * @date Sept 2025
 */
#ifndef MESH_COMPONENT
#define MESH_COMPONENT

// ================================ ( Gareth )
#include<string>
#include<cstdint>
#include<cstddef>

// A handle to a mesh resource (VAO, buffers, etc.)
// This avoids duplicating mesh data for every entity.
struct MeshComponent {

    std::string meshName; // e.g., "Sphere", "Cube", "Extern/tree.obj"

    // Optional: Cache VAO and index count after loading
    uint32_t vao = 0;       // previously was GLuint
    size_t indexCount = 0;

    // temp bounds
    float localMin[3] = { -0.5f, -0.5f, -0.5f };
    float localMax[3] = { +0.5f, +0.5f, +0.5f };
};

#endif