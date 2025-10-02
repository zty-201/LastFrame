
#include "pch.hpp"

#include "Mesh.h"
#include "AssetManager.hpp"
//
    // The Vertex struct.
    // This class is useful for parsing vertex data from OBJ files and converting them into 
    // indices suitable for OpenGL-based rendering or other 3D graphics workflows.
    //
struct Vertex
{
    int pIdx;   // Index for the vertex position in the points array.
    int nIdx;   // Index for the vertex normal in the normals array.
    int tcIdx;  // Index for the texture coordinate in the texCoords array.

    // Constructs a Vertex object by parsing a vertex definition string.
    // This constructor extracts indices for position, texture coordinate, and normal
    // from a vertex definition string in the format typically found in Wavefront OBJ
    // files.
    Vertex(std::string& vertString)
        : pIdx(-1), nIdx(-1), tcIdx(-1)
    {
        size_t slash1, slash2;
        slash1 = vertString.find("/");
        pIdx = std::stoi(vertString.substr(0, slash1)) - 1;
        if (slash1 != std::string::npos)
        {
            slash2 = vertString.find("/", slash1 + 1);
            if (slash2 > slash1 + 1)
                tcIdx = std::stoi(vertString.substr(slash1 + 1, slash2 - slash1 - 1)) - 1;
            nIdx = std::stoi(vertString.substr(slash2 + 1)) - 1;
        }
    }
};

// Generates a single quad (a square plane) with specified side length.
void generateQuad(std::vector<GLuint>& indices,        // Output vector to store vertex indices of the quad.
    std::vector<GLfloat>& points,                     // Output vector to store vertex positions (x, y, z) of the quad.
    std::vector<GLfloat>& normals,                    // Output vector to store normal vectors for the quad.
    std::vector<GLfloat>& texCoords,                  // Output vector to store texture coordinates (u, v) for the quad.
    std::vector<GLfloat>& tangents,                   // Output vector to store tangent vectors for the quad.
    GLfloat side = 2.0f);                             // Optional parameter to specify the side length of the quad. Default is 2.0f.

// Generates a plane consisting of multiple quads in a grid layout.
void generatePlane(std::vector<GLuint>& indices,      // Output vector to store vertex indices of the plane.
    std::vector<GLfloat>& points,                    // Output vector to store vertex positions (x, y, z) of the plane.
    std::vector<GLfloat>& normals,                   // Output vector to store normal vectors for the plane.
    std::vector<GLfloat>& texCoords,                 // Output vector to store texture coordinates (u, v) for the plane.
    std::vector<GLfloat>& tangents);                 // Output vector to store tangent vectors for the plane.

// Generates a cube with the specified size.
void generateCube(std::vector<GLuint>& indices,       // Output vector to store vertex indices of the cube.
    std::vector<GLfloat>& points,                    // Output vector to store vertex positions (x, y, z) of the cube.
    std::vector<GLfloat>& normals,                   // Output vector to store normal vectors for the cube.
    std::vector<GLfloat>& texCoords,                 // Output vector to store texture coordinates (u, v) for the cube.
    std::vector<GLfloat>& tangents,                  // Output vector to store tangent vectors for the cube.
    GLfloat size = 1.0f);                            // Optional parameter to specify the edge length of the cube. Default is 1.0f.

// Generates a torus (a 3D doughnut shape) with customizable dimensions and resolution.
void generateTorus(std::vector<GLuint>& indices,      // Output vector to store vertex indices of the torus.
    std::vector<GLfloat>& points,                    // Output vector to store vertex positions (x, y, z) of the torus.
    std::vector<GLfloat>& normals,                   // Output vector to store normal vectors for the torus.
    std::vector<GLfloat>& texCoords,                 // Output vector to store texture coordinates (u, v) for the torus.
    std::vector<GLfloat>& tangents,                  // Output vector to store tangent vectors for the torus.
    GLfloat outerRadius = 0.7f,                      // Radius from the center of the torus to the center of its tube. Default is 0.7f.
    GLfloat innerRadius = 0.3f,                      // Radius of the tube itself. Default is 0.3f.
    GLuint nsides = 10,                              // Number of subdivisions along the tube's circular cross-section. Default is 10.
    GLuint nrings = 10);                             // Number of subdivisions along the torus' ring. Default is 10.

// Generates a sphere with the specified radius and resolution.
void generateSphere(std::vector<GLuint>& indices,     // Output vector to store vertex indices of the sphere.
    std::vector<GLfloat>& points,                    // Output vector to store vertex positions (x, y, z) of the sphere.
    std::vector<GLfloat>& normals,                   // Output vector to store normal vectors for the sphere.
    std::vector<GLfloat>& texCoords,                 // Output vector to store texture coordinates (u, v) for the sphere.
    std::vector<GLfloat>& tangents,                  // Output vector to store tangent vectors for the sphere.
    float rad = 1.0f,                                // Radius of the sphere. Default is 1.0f.
    GLuint sl = 20,                                  // Number of slices (longitudinal subdivisions). Default is 20.
    GLuint st = 20);                                 // Number of stacks (latitudinal subdivisions). Default is 20.

// Generates a skybox, typically a large cube surrounding the scene.
void generateSkyBox(std::vector<GLuint>& indices,     // Output vector to store vertex indices of the skybox.
    std::vector<GLfloat>& points,                    // Output vector to store vertex positions (x, y, z) of the skybox.
    std::vector<GLfloat>& normals,                   // Output vector to store normal vectors for the skybox.
    std::vector<GLfloat>& texCoords,                 // Output vector to store texture coordinates (u, v) for the skybox.
    std::vector<GLfloat>& tangents,                  // Output vector to store tangent vectors for the skybox.
    GLfloat size = 100.0f);                          // Size of the skybox cube. Default is 100.0f.

// Generates a mesh from an external file, typically an OBJ or similar format.
void generate(const char* fileName,                  // Path to the mesh file.
    std::vector<GLuint>& indices,                    // Output vector to store vertex indices of the mesh.
    std::vector<GLfloat>& points,                    // Output vector to store vertex positions (x, y, z) of the mesh.
    std::vector<GLfloat>& normals,                   // Output vector to store normal vectors for the mesh.
    std::vector<GLfloat>& texCoords,                 // Output vector to store texture coordinates (u, v) of the mesh.
    std::vector<GLfloat>& tangents,                  // Output vector to store tangent vectors for the mesh.
    bool genTangents = false);                       // Whether to generate tangent vectors. Default is false.

// Constructor for the TriangleMesh class. Depending on the value of the input fileName, it generates
// a specific type of 3D geometry (e.g., Quad, Plane, Cube, etc.) or loads a mesh from a file.
TriangleMesh::TriangleMesh(const char* fileName /*= ""*/)
    : indices{ }, points{ }, normals{ }, texCoords{ }, tangents{ }
{
    if (!strcmp(fileName, "Quad"))
        generateQuad(indices, points, normals, texCoords, tangents);
    else if (!strcmp(fileName, "Plane"))
        generatePlane(indices, points, normals, texCoords, tangents);
    else if (!strcmp(fileName, "Cube"))
        generateCube(indices, points, normals, texCoords, tangents);
    else if (!strcmp(fileName, "Torus"))
        generateTorus(indices, points, normals, texCoords, tangents);
    else if (!strcmp(fileName, "Sphere"))
        generateSphere(indices, points, normals, texCoords, tangents);
    else if (!strcmp(fileName, "SkyBox"))
        generateSkyBox(indices, points, normals, texCoords, tangents);
    else if (fileName)
        generate(fileName, indices, points, normals, texCoords, tangents, true);
}

// Generates a single quad (square plane) centered at the origin.
// The quad lies in the XY plane with a normal pointing along the positive Z-axis.
void generateQuad(std::vector<GLuint>& indices,         // Output vector to store vertex indices.
    std::vector<GLfloat>& points,                      // Output vector to store vertex positions (x, y, z).
    std::vector<GLfloat>& normals,                     // Output vector to store normal vectors (x, y, z).
    std::vector<GLfloat>& texCoords,                   // Output vector to store texture coordinates (u, v).
    std::vector<GLfloat>& tangents,                    // Output vector to store tangent vectors.
    GLfloat side /*= 2.0f*/)                           // Optional parameter specifying the length of each side. Default is 2.0f.
{
    UNREFERENCED_PARAMETER(tangents);
    const GLfloat side2 = side / 2.0f; // Calculate half the side length for positioning the vertices.

    // Define the vertex positions of the quad (counterclockwise order).
    points = {
        -side2, -side2, 0.0f, // Bottom-left vertex
         side2, -side2, 0.0f, // Bottom-right vertex
         side2,  side2, 0.0f, // Top-right vertex
        -side2,  side2, 0.0f  // Top-left vertex
    };

    // Define the normal vectors for the quad (all pointing along the positive Z-axis).
    normals = {
        0.0f, 0.0f, 1.0f, // Bottom-left vertex
        0.0f, 0.0f, 1.0f, // Bottom-right vertex
        0.0f, 0.0f, 1.0f, // Top-right vertex
        0.0f, 0.0f, 1.0f  // Top-left vertex
    };

    // Define the texture coordinates for the quad.
    texCoords = {
        0.0f, 0.0f, // Bottom-left vertex
        1.0f, 0.0f, // Bottom-right vertex
        1.0f, 1.0f, // Top-right vertex
        0.0f, 1.0f  // Top-left vertex
    };

    // Define the vertex indices for two triangles forming the quad.
    indices = {
        0, 1, 2, // First triangle (bottom-left, bottom-right, top-right)
        0, 2, 3  // Second triangle (bottom-left, top-right, top-left)
    };

    // Tangents are not calculated in this function. They can be generated later if required.
}


// Generates a plane composed of 9 vertices arranged in a 3x3 grid.
// The plane lies in the XZ plane with its normal pointing along the positive Y-axis.
void generatePlane(std::vector<GLuint>& indices,        // Output vector to store vertex indices.
    std::vector<GLfloat>& points,       // Output vector to store vertex positions (x, y, z).
    std::vector<GLfloat>& normals,      // Output vector to store normal vectors (x, y, z).
    std::vector<GLfloat>& texCoords,    // Output vector to store texture coordinates (u, v).
    std::vector<GLfloat>& tangents)     // Output vector to store tangent vectors.
{
    UNREFERENCED_PARAMETER(tangents);
    const GLfloat side = 4.0f; // Define the side length of the grid spacing in both X and Z directions.

    // Define vertex positions for the plane (3x3 grid of vertices in the XZ plane, with Y=0).
    points = {
       -side,  0.0f,   -side,  // Bottom-left
           0,  0.0f,   -side,  // Bottom-center
        side,  0.0f,   -side,  // Bottom-right
       -side,  0.0f,    0.0f,  // Middle-left
           0,  0.0f,    0.0f,  // Center
        side,  0.0f,    0.0f,  // Middle-right
       -side,  0.0f,    side,  // Top-left
           0,  0.0f,    side,  // Top-center
        side,  0.0f,    side   // Top-right
    };

    // Define the normal vectors for each vertex (all pointing up along the Y-axis).
    normals = {
        0.0f, 1.0f, 0.0f,  // Bottom-left
        0.0f, 1.0f, 0.0f,  // Bottom-center
        0.0f, 1.0f, 0.0f,  // Bottom-right
        0.0f, 1.0f, 0.0f,  // Middle-left
        0.0f, 1.0f, 0.0f,  // Center
        0.0f, 1.0f, 0.0f,  // Middle-right
        0.0f, 1.0f, 0.0f,  // Top-left
        0.0f, 1.0f, 0.0f,  // Top-center
        0.0f, 1.0f, 0.0f   // Top-right
    };

    // Define the texture coordinates for each vertex (spanning the range [0, 1] in the UV space).
    texCoords = {
        0.0f, 0.0f,  // Bottom-left
        0.5f, 0.0f,  // Bottom-center
        1.0f, 0.0f,  // Bottom-right
        0.0f, 0.5f,  // Middle-left
        0.5f, 0.5f,  // Center
        1.0f, 0.5f,  // Middle-right
        0.0f, 1.0f,  // Top-left
        0.5f, 1.0f,  // Top-center
        1.0f, 1.0f   // Top-right
    };

    // Define the vertex indices to form the plane using 8 triangles (in counterclockwise order).
    // The grid layout is as follows (vertex indices):
    // 6 7 8
    // 3 4 5
    // 0 1 2
    indices = {
        0, 1, 3,  // Bottom-left triangle
        3, 1, 4,  // Middle-left triangle
        4, 1, 2,  // Bottom-center triangle
        2, 5, 4,  // Bottom-right triangle
        3, 4, 6,  // Top-left triangle
        6, 4, 7,  // Middle-left triangle
        7, 4, 5,  // Top-center triangle
        5, 8, 7   // Top-right triangle
    };

    // Tangents are not explicitly calculated in this function.
}


// Generates a cube with specified side length.
// The cube is centered at the origin, with each face consisting of 2 triangles.
// Each face has unique normals and texture coordinates.
void generateCube(std::vector<GLuint>& indices,       // Output vector for vertex indices.
    std::vector<GLfloat>& points,      // Output vector for vertex positions (x, y, z).
    std::vector<GLfloat>& normals,     // Output vector for normal vectors (x, y, z).
    std::vector<GLfloat>& texCoords,   // Output vector for texture coordinates (u, v).
    std::vector<GLfloat>& tangents,    // Output vector for tangent vectors (not calculated here).
    GLfloat side /*= 1.0f*/)           // Length of each side of the cube.
{
    UNREFERENCED_PARAMETER(tangents);
    // Define vertex positions for each face of the cube.
    // Each face is defined by four vertices in counter-clockwise order.
    points = {
        // Front face
       -side, -side,  side,   // Bottom-left
        side, -side,  side,   // Bottom-right
        side,  side,  side,   // Top-right
       -side,  side,  side,   // Top-left
       // Right face
       side, -side,  side,   // Bottom-left
       side, -side, -side,   // Bottom-right
       side,  side, -side,   // Top-right
       side,  side,  side,   // Top-left
       // Back face
      -side, -side, -side,   // Bottom-left
      -side,  side, -side,   // Top-left
       side,  side, -side,   // Top-right
       side, -side, -side,   // Bottom-right
       // Left face
      -side, -side,  side,   // Bottom-right
      -side,  side,  side,   // Top-right
      -side,  side, -side,   // Top-left
      -side, -side, -side,   // Bottom-left
      // Bottom face
     -side, -side,  side,   // Front-left
     -side, -side, -side,   // Back-left
      side, -side, -side,   // Back-right
      side, -side,  side,   // Front-right
      // Top face
     -side,  side,  side,   // Front-left
      side,  side,  side,   // Front-right
      side,  side, -side,   // Back-right
     -side,  side, -side    // Back-left
    };

    // Define normal vectors for each face of the cube.
    // Each face has its own unique normal direction.
    normals = {
        // Front face
        0.0f, 0.0f,  1.0f, 0.0f, 0.0f,  1.0f, 0.0f, 0.0f,  1.0f, 0.0f, 0.0f,  1.0f,
        // Right face
        1.0f, 0.0f,  0.0f, 1.0f, 0.0f,  0.0f, 1.0f, 0.0f,  0.0f, 1.0f, 0.0f,  0.0f,
        // Back face
        0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f,
        // Left face
       -1.0f, 0.0f,  0.0f, -1.0f, 0.0f,  0.0f, -1.0f, 0.0f,  0.0f, -1.0f, 0.0f,  0.0f,
       // Bottom face
       0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f,
       // Top face
       0.0f,  1.0f, 0.0f, 0.0f,  1.0f, 0.0f, 0.0f,  1.0f, 0.0f, 0.0f,  1.0f, 0.0f
    };

    // Define texture coordinates for each face.
    // The same UV mapping is used for all six faces.
    texCoords = {
        // Front face
        0.0f, 0.0f,  1.0f, 0.0f,  1.0f, 1.0f,  0.0f, 1.0f,
        // Right face
        0.0f, 0.0f,  1.0f, 0.0f,  1.0f, 1.0f,  0.0f, 1.0f,
        // Back face
        0.0f, 0.0f,  1.0f, 0.0f,  1.0f, 1.0f,  0.0f, 1.0f,
        // Left face
        0.0f, 0.0f,  1.0f, 0.0f,  1.0f, 1.0f,  0.0f, 1.0f,
        // Bottom face
        0.0f, 0.0f,  1.0f, 0.0f,  1.0f, 1.0f,  0.0f, 1.0f,
        // Top face
        0.0f, 0.0f,  1.0f, 0.0f,  1.0f, 1.0f,  0.0f, 1.0f
    };

    // Define vertex indices for each face of the cube.
    // Each face is formed by two triangles (6 indices per face).
    indices = {
        0, 1, 2,  0, 2, 3,  // Front face
        4, 5, 6,  4, 6, 7,  // Right face
        8, 9, 10, 8, 10,11, // Back face
        12,13,14, 12,14,15, // Left face
        16,17,18, 16,18,19, // Bottom face
        20,21,22, 20,22,23  // Top face
    };

    // Note: Tangents are not calculated here, but the vector is prepared to be populated if needed.
}


// Generates a torus (donut-shaped 3D object) with specified parameters.
// The torus is constructed as a mesh of triangles.
void generateTorus(std::vector<GLuint>& indices,        // Output vector for vertex indices.
    std::vector<GLfloat>& points,       // Output vector for vertex positions (x, y, z).
    std::vector<GLfloat>& normals,      // Output vector for normal vectors (x, y, z).
    std::vector<GLfloat>& texCoords,    // Output vector for texture coordinates (u, v).
    std::vector<GLfloat>& tangents,     // Output vector for tangent vectors (not calculated here).
    GLfloat outerRadius /*= 0.7f*/,     // Radius of the torus ring (distance from the center to the tube center).
    GLfloat innerRadius /*= 0.3f*/,     // Radius of the torus tube.
    GLuint nsides /*= 10*/,             // Number of sides (segments around the tube).
    GLuint nrings /*= 10*/)             // Number of rings (segments around the torus).
{
    UNREFERENCED_PARAMETER(tangents);
    const int nFaces = nsides * nrings;                // Total number of faces (quads).
    const int nVerts = nsides * (nrings + 1);          // Total number of vertices (extra ring to duplicate the first ring).

    // Resize output vectors to accommodate the required data.
    points.resize(3 * nVerts);                         // Each vertex has 3 coordinates (x, y, z).
    normals.resize(3 * nVerts);                        // Each vertex has 3 normal components (x, y, z).
    texCoords.resize(2 * nVerts);                      // Each vertex has 2 texture coordinates (u, v).
    indices.resize(6 * nFaces);                        // Each face (quad) consists of 2 triangles (6 indices).

    // Factors to calculate angles for rings and sides.
    float ringFactor = glm::two_pi<float>() / nrings;  // Angle increment per ring.
    float sideFactor = glm::two_pi<float>() / nsides;  // Angle increment per side.

    // Generate the vertex data.
    int idx = 0;                                       // Index for position and normal data.
    int tidx = 0;                                      // Index for texture coordinate data.
    for (GLuint ring = 0; ring <= nrings; ring++)      // Iterate through each ring (include extra ring).
    {
        float u = ring * ringFactor;                   // Current angle around the torus ring.
        float cu = cosf(u);                            // Cosine of the angle for x and z.
        float su = sinf(u);                            // Sine of the angle for x and z.

        for (GLuint side = 0; side < nsides; side++)   // Iterate through each segment of the torus tube.
        {
            float v = side * sideFactor;               // Current angle around the tube.
            float cv = cosf(v);                        // Cosine for tube radius.
            float sv = sinf(v);                        // Sine for tube radius.
            float r = outerRadius + innerRadius * cv;  // Radial position of the vertex.

            // Calculate vertex position.
            points[idx] = r * cu;                      // x-coordinate.
            points[idx + 1] = innerRadius * sv;        // y-coordinate.
            points[idx + 2] = r * su;                  // z-coordinate.

            // Calculate normal vector.
            normals[idx] = cv * cu * r;                // Normal x-component.
            normals[idx + 1] = sv * r;                 // Normal y-component.
            normals[idx + 2] = cv * su * r;            // Normal z-component.

            // Calculate texture coordinates.
            texCoords[tidx] = u / glm::two_pi<float>(); // Texture u-coordinate (ring).
            texCoords[tidx + 1] = v / glm::two_pi<float>(); // Texture v-coordinate (side).

            tidx += 2;                                 // Advance texture coordinate index.

            // Normalize the normal vector.
            float len = sqrtf(normals[idx] * normals[idx] +
                normals[idx + 1] * normals[idx + 1] +
                normals[idx + 2] * normals[idx + 2]); // Length of the normal vector.
            normals[idx] /= len;                       // Normalize x-component.
            normals[idx + 1] /= len;                   // Normalize y-component.
            normals[idx + 2] /= len;                   // Normalize z-component.

            idx += 3;                                  // Advance position/normal index.
        }
    }

    // Generate the index data for the mesh.
    idx = 0;
    for (GLuint ring = 0; ring < nrings; ring++)       // Iterate through each ring.
    {
        GLuint ringStart = ring * nsides;              // Starting index of the current ring.
        GLuint nextRingStart = (ring + 1) * nsides;    // Starting index of the next ring.

        for (GLuint side = 0; side < nsides; side++)   // Iterate through each segment of the tube.
        {
            int nextSide = (side + 1) % nsides;        // Wrap around for the last segment.

            // Define the two triangles for the current quad.
            indices[idx] = ringStart + side;           // First triangle: bottom-left.
            indices[idx + 1] = nextRingStart + side;   // First triangle: top-left.
            indices[idx + 2] = nextRingStart + nextSide; // First triangle: top-right.

            indices[idx + 3] = ringStart + side;       // Second triangle: bottom-left.
            indices[idx + 4] = nextRingStart + nextSide; // Second triangle: top-right.
            indices[idx + 5] = ringStart + nextSide;   // Second triangle: bottom-right.

            idx += 6;                                  // Advance index count.
        }
    }

    // Note: Tangents are not calculated here, but the vector is prepared to be populated if needed.
}


// Generates a sphere mesh with specified parameters.
// The sphere is constructed as a mesh of triangles.
void generateSphere(std::vector<GLuint>& indices,        // Output vector for vertex indices.
    std::vector<GLfloat>& points,       // Output vector for vertex positions (x, y, z).
    std::vector<GLfloat>& normals,      // Output vector for normal vectors (x, y, z).
    std::vector<GLfloat>& texCoords,    // Output vector for texture coordinates (u, v).
    std::vector<GLfloat>& tangents,     // Output vector for tangent vectors (not calculated here).
    float radius /*= 1.0f*/,           // Radius of the sphere.
    GLuint nSlices /*= 20*/,           // Number of slices (longitudinal divisions).
    GLuint nStacks /*= 20*/)           // Number of stacks (latitudinal divisions).
{
    UNREFERENCED_PARAMETER(tangents);
    // Calculate the total number of vertices and faces.
    const int nVerts = (nSlices + 1) * (nStacks + 1);    // Total number of vertices (include duplicate for seamless wrapping).
    const int nFaces = (nSlices * 2 * (nStacks - 1)) * 3; // Total number of face indices (triangles).

    // Resize the output vectors to hold the required data.
    points.resize(3 * nVerts);                           // Each vertex has 3 coordinates (x, y, z).
    normals.resize(3 * nVerts);                          // Each vertex has 3 normal components (x, y, z).
    texCoords.resize(2 * nVerts);                        // Each vertex has 2 texture coordinates (u, v).
    indices.resize(nFaces);                              // Total number of indices for the faces.

    // Factors to calculate angles for slices (longitude) and stacks (latitude).
    GLfloat thetaFac = glm::two_pi<float>() / nSlices;   // Increment for theta (longitude angle).
    GLfloat phiFac = glm::pi<float>() / nStacks;         // Increment for phi (latitude angle).

    // Variables for position, normal, and texture coordinate calculations.
    GLfloat theta, phi;                                  // Longitude and latitude angles.
    GLfloat nx, ny, nz;                                  // Components of the normal vector.
    GLfloat s, t;                                        // Texture coordinates.
    GLuint idx = 0, tIdx = 0;                            // Indices for vertex and texture data.

    // Generate vertex data.
    for (GLuint i = 0; i <= nSlices; i++) {              // Iterate through slices (longitude).
        theta = i * thetaFac;                            // Calculate current longitude angle.
        s = (GLfloat)i / nSlices;                        // Texture coordinate (s).

        for (GLuint j = 0; j <= nStacks; j++) {          // Iterate through stacks (latitude).
            phi = j * phiFac;                            // Calculate current latitude angle.
            t = (GLfloat)j / nStacks;                    // Texture coordinate (t).

            // Calculate the normal vector.
            nx = sinf(phi) * cosf(theta);                // Normal x-component.
            ny = sinf(phi) * sinf(theta);                // Normal y-component.
            nz = cosf(phi);                              // Normal z-component.

            // Calculate vertex position by scaling the normal by the radius.
            points[idx] = radius * nx;                   // x-coordinate.
            points[idx + 1] = radius * ny;               // y-coordinate.
            points[idx + 2] = radius * nz;               // z-coordinate.

            // Store the normalized normal vector.
            normals[idx] = nx;                           // Normal x-component.
            normals[idx + 1] = ny;                       // Normal y-component.
            normals[idx + 2] = nz;                       // Normal z-component.

            idx += 3;                                    // Advance vertex index.

            // Store texture coordinates.
            texCoords[tIdx] = s;                         // u-coordinate.
            texCoords[tIdx + 1] = t;                     // v-coordinate.

            tIdx += 2;                                   // Advance texture coordinate index.
        }
    }

    // Generate element (index) list for faces.
    idx = 0;
    for (GLuint i = 0; i < nSlices; i++) {               // Iterate through slices (longitude).
        GLuint stackStart = i * (nStacks + 1);           // Starting index of the current stack.
        GLuint nextStackStart = (i + 1) * (nStacks + 1); // Starting index of the next stack.

        for (GLuint j = 0; j < nStacks; j++) {           // Iterate through stacks (latitude).

            // Handle the first stack (triangles at the pole).
            if (j == 0) {
                indices[idx] = stackStart;               // Vertex at the pole.
                indices[idx + 1] = stackStart + 1;       // First vertex of the triangle.
                indices[idx + 2] = nextStackStart + 1;   // Second vertex of the triangle.
                idx += 3;                                // Advance index count.
            }
            // Handle the last stack (triangles near the pole).
            else if (j == nStacks - 1) {
                indices[idx] = stackStart + j;           // First vertex of the triangle.
                indices[idx + 1] = stackStart + j + 1;   // Second vertex of the triangle.
                indices[idx + 2] = nextStackStart + j;   // Vertex at the pole.
                idx += 3;                                // Advance index count.
            }
            // Handle the middle stacks (quads split into two triangles).
            else {
                // First triangle of the quad.
                indices[idx] = stackStart + j;
                indices[idx + 1] = stackStart + j + 1;
                indices[idx + 2] = nextStackStart + j + 1;

                // Second triangle of the quad.
                indices[idx + 3] = nextStackStart + j;
                indices[idx + 4] = stackStart + j;
                indices[idx + 5] = nextStackStart + j + 1;

                idx += 6;                                // Advance index count.
            }
        }
    }

    // Note: Tangents are not calculated here, but the vector is prepared to be populated if needed.
}


// Generates a cube-shaped skybox with specified side length.
// The skybox is constructed as a set of 6 faces (quads) and rendered using triangles.
void generateSkyBox(std::vector<GLuint>& indices,        // Output vector for vertex indices.
    std::vector<GLfloat>& points,       // Output vector for vertex positions (x, y, z).
    std::vector<GLfloat>& normals,      // Output vector for normal vectors (not used for skybox).
    std::vector<GLfloat>& texCoords,    // Output vector for texture coordinates (not used for skybox).
    std::vector<GLfloat>& tangents,     // Output vector for tangent vectors (not calculated here).
    GLfloat side /*= 50.0f*/)           // Length of a side of the cube.
{
    UNREFERENCED_PARAMETER(tangents);
    UNREFERENCED_PARAMETER(texCoords);
    // Half the length of the side, used to center the cube at the origin.
    const GLfloat side2 = side / 2.0f;

    // Define the vertex positions for the 6 faces of the cube.
    // Each face is defined as a quad made of 4 vertices.
    points = {
        // Front face
       -side2, -side2,  side2,   // Bottom-left
        side2, -side2,  side2,   // Bottom-right
        side2,  side2,  side2,   // Top-right
       -side2,  side2,  side2,   // Top-left

       // Right face
       side2, -side2,  side2,   // Bottom-front
       side2, -side2, -side2,   // Bottom-back
       side2,  side2, -side2,   // Top-back
       side2,  side2,  side2,   // Top-front

       // Back face
      -side2, -side2, -side2,   // Bottom-right
      -side2,  side2, -side2,   // Top-right
       side2,  side2, -side2,   // Top-left
       side2, -side2, -side2,   // Bottom-left

       // Left face
      -side2, -side2,  side2,   // Bottom-front
      -side2,  side2,  side2,   // Top-front
      -side2,  side2, -side2,   // Top-back
      -side2, -side2, -side2,   // Bottom-back

      // Bottom face
     -side2, -side2,  side2,   // Front-left
     -side2, -side2, -side2,   // Back-left
      side2, -side2, -side2,   // Back-right
      side2, -side2,  side2,   // Front-right

      // Top face
     -side2,  side2,  side2,   // Front-left
      side2,  side2,  side2,   // Front-right
      side2,  side2, -side2,   // Back-right
     -side2,  side2, -side2    // Back-left
    };

    // Normals are not used for a skybox as it is typically rendered without shading.
    // Fill the normals vector with zeroes (placeholder).
    normals.resize(points.size(), 0.0f);

    // Define the indices for rendering each face of the cube using two triangles.
    // Each face consists of 6 indices (2 triangles per face).
    indices = {
        // Front face
        0, 2, 1,    // Triangle 1
        0, 3, 2,    // Triangle 2

        // Right face
        4, 6, 5,    // Triangle 1
        4, 7, 6,    // Triangle 2

        // Back face
        8, 10, 9,   // Triangle 1
        8, 11, 10,  // Triangle 2

        // Left face
        12, 14, 13, // Triangle 1
        12, 15, 14, // Triangle 2

        // Bottom face
        16, 18, 17, // Triangle 1
        16, 19, 18, // Triangle 2

        // Top face
        20, 22, 21, // Triangle 1
        20, 23, 22  // Triangle 2
    };

    // Note: Texture coordinates and tangents are not calculated here, as skyboxes generally do not require them.
}

void generate(const char* fileName,
    std::vector<GLuint>& indices,
    std::vector<GLfloat>& points,
    std::vector<GLfloat>& normals,
    std::vector<GLfloat>& texCoords,
    std::vector<GLfloat>& tangents,
    bool genTangents /*= false*/)
{

    struct MeshData {
        std::vector <glm::vec3> points;
        std::vector <glm::vec3> normals;
        std::vector <glm::vec2> texCoords;
        std::vector <Vertex> faces;
        std::vector <glm::vec4> tangents;
    } meshData;

    // Load data
    std::vector<unsigned char> temp = EngineFramework::AssetManagerSystem::GetDataFromFile(fileName);
    /*if (!objStream)    exception thrown by GetData already.
        throw std::runtime_error(std::string("file ") + fileName + " not found.");*/
    std::string file_string{ temp.begin(), temp.end() };
    std::stringstream objStream(file_string);

   

    std::string line, token;
    getline(objStream, line);
    while (!objStream.eof())
    {
        // Remove comment if it exists
        size_t pos = line.find_first_of("#");
        if (pos != std::string::npos)
            line = line.substr(0, pos);

        // Trim string
        const char* whiteSpace = " \t\n\r";
        size_t location;
        location = line.find_first_not_of(whiteSpace);
        line.erase(0, location);
        location = line.find_last_not_of(whiteSpace);
        line.erase(location + 1);

        if (line.length() > 0) {
            std::istringstream lineStream(line);

            lineStream >> token;

            if (token == "v") {
                float x, y, z;
                lineStream >> x >> y >> z;
                glm::vec3 p(x, y, z);
                meshData.points.push_back(p);
            }
            else if (token == "vt") {
                // Process texture coordinate
                float s, t;
                lineStream >> s >> t;
                meshData.texCoords.push_back(glm::vec2(s, t));
            }
            else if (token == "vn") {
                float x, y, z;
                lineStream >> x >> y >> z;
                meshData.normals.push_back(glm::vec3(x, y, z));
            }
            else if (token == "f") {
                std::vector<std::string> parts;
                while (lineStream.good()) {
                    std::string s;
                    lineStream >> s;
                    parts.push_back(s);
                }

                // Triangulate as a triangle fan
                if (parts.size() > 2) {
                    Vertex firstVert(parts[0]);
                    for (int i = 2; i < parts.size(); i++) {
                        meshData.faces.push_back(firstVert);
                        meshData.faces.push_back(Vertex(parts[i - 1]));
                        meshData.faces.push_back(Vertex(parts[i]));
                    }
                }
            }
        }
        getline(objStream, line);
    }

    // Generate normals if needed
    if (meshData.normals.size() == 0)
    {
        meshData.normals.resize(meshData.points.size());

        for (GLuint i = 0; i < meshData.faces.size(); i += 3)
        {
            const glm::vec3& p1 = meshData.points[meshData.faces[i].pIdx];
            const glm::vec3& p2 = meshData.points[meshData.faces[i + 1].pIdx];
            const glm::vec3& p3 = meshData.points[meshData.faces[i + 2].pIdx];

            glm::vec3 a = p2 - p1;
            glm::vec3 b = p3 - p1;
            glm::vec3 n = glm::normalize(glm::cross(a, b));

            meshData.normals[meshData.faces[i].pIdx] += n;
            meshData.normals[meshData.faces[i + 1].pIdx] += n;
            meshData.normals[meshData.faces[i + 2].pIdx] += n;

            // Set the normal index to be the same as the point index
            meshData.faces[i].nIdx = meshData.faces[i].pIdx;
            meshData.faces[i + 1].nIdx = meshData.faces[i + 1].pIdx;
            meshData.faces[i + 2].nIdx = meshData.faces[i + 2].pIdx;
        }

        for (GLuint i = 0; i < meshData.normals.size(); i++)
            meshData.normals[i] = glm::normalize(meshData.normals[i]);
    }

    // Generate tangents if needed
    if (genTangents)
    {
        std::vector<glm::vec3> tan1Accum(meshData.points.size());
        std::vector<glm::vec3> tan2Accum(meshData.points.size());
        meshData.tangents.resize(meshData.points.size());

        // Compute the tangent std::vector
        for (GLuint i = 0; i < meshData.faces.size(); i += 3)
        {
            const glm::vec3& p1 = meshData.points[meshData.faces[i].pIdx];
            const glm::vec3& p2 = meshData.points[meshData.faces[i + 1].pIdx];
            const glm::vec3& p3 = meshData.points[meshData.faces[i + 2].pIdx];

            const glm::vec2& tc1 = meshData.texCoords[meshData.faces[i].tcIdx];
            const glm::vec2& tc2 = meshData.texCoords[meshData.faces[i + 1].tcIdx];
            const glm::vec2& tc3 = meshData.texCoords[meshData.faces[i + 2].tcIdx];

            glm::vec3 q1 = p2 - p1;
            glm::vec3 q2 = p3 - p1;
            float s1 = tc2.x - tc1.x, s2 = tc3.x - tc1.x;
            float t1 = tc2.y - tc1.y, t2 = tc3.y - tc1.y;
            float r = 1.0f / (s1 * t2 - s2 * t1);
            glm::vec3 tan1((t2 * q1.x - t1 * q2.x) * r,
                (t2 * q1.y - t1 * q2.y) * r,
                (t2 * q1.z - t1 * q2.z) * r);
            glm::vec3 tan2((s1 * q2.x - s2 * q1.x) * r,
                (s1 * q2.y - s2 * q1.y) * r,
                (s1 * q2.z - s2 * q1.z) * r);
            tan1Accum[meshData.faces[i].pIdx] += tan1;
            tan1Accum[meshData.faces[i + 1].pIdx] += tan1;
            tan1Accum[meshData.faces[i + 2].pIdx] += tan1;
            tan2Accum[meshData.faces[i].pIdx] += tan2;
            tan2Accum[meshData.faces[i + 1].pIdx] += tan2;
            tan2Accum[meshData.faces[i + 2].pIdx] += tan2;
        }

        for (GLuint i = 0; i < meshData.points.size(); ++i)
        {
            const glm::vec3& n = meshData.normals[i];
            glm::vec3& t1 = tan1Accum[i];
            glm::vec3& t2 = tan2Accum[i];

            // Gram-Schmidt orthogonalize
            meshData.tangents[i] = glm::vec4(glm::normalize(t1 - (glm::dot(n, t1) * n)), 0.0f);
            // Store handedness in w
            meshData.tangents[i].w = (glm::dot(glm::cross(n, t1), t2) < 0.0f) ? -1.0f : 1.0f;
        }
    }

    // Convert to GL format
    std::map<std::string, GLuint> vertexMap;
    for (auto& vert : meshData.faces)
    {
        std::string vertStr = std::to_string(vert.pIdx) + "/" +
            std::to_string(vert.tcIdx) + "/" +
            std::to_string(vert.nIdx);
        auto it = vertexMap.find(vertStr);
        if (it == vertexMap.end()) {
            auto vIdx = points.size() / 3;

            auto& pt = meshData.points[vert.pIdx];
            points.push_back(pt.x);
            points.push_back(pt.y);
            points.push_back(pt.z);

            auto& n = meshData.normals[vert.nIdx];
            normals.push_back(n.x);
            normals.push_back(n.y);
            normals.push_back(n.z);

            if (!meshData.texCoords.empty()) {
                auto& tc = meshData.texCoords[vert.tcIdx];
                texCoords.push_back(tc.x);
                texCoords.push_back(tc.y);
            }

            if (!meshData.tangents.empty()) {
                // We use the point index for tangents
                auto& tang = meshData.tangents[vert.pIdx];
                tangents.push_back(tang.x);
                tangents.push_back(tang.y);
                tangents.push_back(tang.z);
                tangents.push_back(tang.w);
            }

            indices.push_back((GLuint)vIdx);
            vertexMap[vertStr] = (GLuint)vIdx;
        }
        else
            indices.push_back(it->second);
    }

}
