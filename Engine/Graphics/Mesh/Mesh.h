#ifndef MESH_H
#define MESH_H

//
    //  Represents a 3D triangle mesh for rendering and scene definitions.
    // 
    //  The TriangleMesh structure encapsulates the geometry data required to define
    //  and render a triangle mesh in a 3D scene, including vertices, normals, texture
    //  coordinates, and tangents. This data is typically used in rendering pipelines
    //  with OpenGL.
    //
struct TriangleMesh
{
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

    //
    // Constructs a TriangleMesh object.
    //
    // Optionally, a file name can be provided to initialize the mesh from
    // an external resource. The exact loading mechanism is not defined in
    // this declaration and would typically be implemented in the corresponding
    // source file.
    //
    TriangleMesh(const char* fileName = "");
};
#endif // !MESH_H