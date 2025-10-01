#ifndef MESH_COMPONENT
#define MESH_COMPONENT


// A handle to a mesh resource (VAO, buffers, etc.)
// This avoids duplicating mesh data for every entity.
struct MeshComponent {

    // Indices defining the connectivity of the mesh. Each group of three indices
    // corresponds to a single triangle.
    std::vector<GLuint> indices;

    // Vertex positions of the mesh. Each set of three consecutive floats represents
    // a single vertex position (x, y, z).
    std::vector<GLfloat> points;

    // Vertex normals used for lighting calculations. Each set of three consecutive
    // floats represents a normal vector (nx, ny, nz).
    std::vector<GLfloat> normals;

    // Texture coordinates for mapping textures to the mesh surface. Each set of
    // two consecutive floats represents a texture coordinate (u, v).
    std::vector<GLfloat> texCoords;

    // Tangent vectors for each vertex. These are used in advanced lighting techniques,
    // such as normal mapping, to compute tangent-space transformations. Each set of
    // three consecutive floats represents a tangent vector (tx, ty, tz).
    std::vector<GLfloat> tangents;

};
#endif