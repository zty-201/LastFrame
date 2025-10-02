#ifndef MESH_COMPONENT
#define MESH_COMPONENT


// A handle to a mesh resource (VAO, buffers, etc.)
// This avoids duplicating mesh data for every entity.
struct MeshComponent {

    std::string meshName; // e.g., "Sphere", "Cube", "Extern/tree.obj"

    // Optional: Cache VAO and index count after loading
    GLuint vao = 0;
    size_t indexCount = 0;

};
#endif