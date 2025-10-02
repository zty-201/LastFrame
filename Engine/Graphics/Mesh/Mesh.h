
#ifndef MESH_H
#define MESH_H

namespace Mesh {
    struct MeshData {
        std::vector<GLfloat> points;     // Vertex positions (x, y, z)
        std::vector<GLfloat> normals;    // Vertex normals (x, y, z)
        std::vector<GLfloat> texCoords;  // Texture coordinates (u, v)
        std::vector<GLfloat> tangents;   // Tangent vectors (x, y, z)
		std::vector<GLuint> indices;     // Indices for indexed drawing
    };
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

    // Generates a single quad (square plane) centered at the origin.
    // The quad lies in the XY plane with a normal pointing along the positive Z-axis.
    inline void generateQuad(MeshData& mesh, GLfloat side = 2.0f)                           // Optional parameter specifying the length of each side. Default is 2.0f.
    {
        const GLfloat side2 = side / 2.0f; // Calculate half the side length for positioning the vertices.

        // Define the vertex positions of the quad (counterclockwise order).
        mesh.points = {
            -side2, -side2, 0.0f, // Bottom-left vertex
             side2, -side2, 0.0f, // Bottom-right vertex
             side2,  side2, 0.0f, // Top-right vertex
            -side2,  side2, 0.0f  // Top-left vertex
        };

        // Define the normal vectors for the quad (all pointing along the positive Z-axis).
        mesh.normals = {
            0.0f, 0.0f, 1.0f, // Bottom-left vertex
            0.0f, 0.0f, 1.0f, // Bottom-right vertex
            0.0f, 0.0f, 1.0f, // Top-right vertex
            0.0f, 0.0f, 1.0f  // Top-left vertex
        };

        // Define the texture coordinates for the quad.
        mesh.texCoords = {
            0.0f, 0.0f, // Bottom-left vertex
            1.0f, 0.0f, // Bottom-right vertex
            1.0f, 1.0f, // Top-right vertex
            0.0f, 1.0f  // Top-left vertex
        };

        // Define the vertex indices for two triangles forming the quad.
        mesh.indices = {
            0, 1, 2, // First triangle (bottom-left, bottom-right, top-right)
            0, 2, 3  // Second triangle (bottom-left, top-right, top-left)
        };

    }


    // Generates a plane composed of 9 vertices arranged in a 3x3 grid.
    // The plane lies in the XZ plane with its normal pointing along the positive Y-axis.
    inline void generatePlane(MeshData& mesh)     // Output vector to store tangent vectors.
    {
        const GLfloat side = 4.0f; // Define the side length of the grid spacing in both X and Z directions.

        // Define vertex positions for the plane (3x3 grid of vertices in the XZ plane, with Y=0).
        mesh.points = {
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
        mesh.normals = {
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
        mesh.texCoords = {
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
        mesh.indices = {
            0, 1, 3,  // Bottom-left triangle
            3, 1, 4,  // Middle-left triangle
            4, 1, 2,  // Bottom-center triangle
            2, 5, 4,  // Bottom-right triangle
            3, 4, 6,  // Top-left triangle
            6, 4, 7,  // Middle-left triangle
            7, 4, 5,  // Top-center triangle
            5, 8, 7   // Top-right triangle
        };

    }


    // Generates a cube with specified side length.
    // The cube is centered at the origin, with each face consisting of 2 triangles.
    // Each face has unique normals and texture coordinates.
    inline void generateCube(MeshData& mesh, GLfloat side = 1.0f)           // Length of each side of the cube.
    {
        // Define vertex positions for each face of the cube.
        // Each face is defined by four vertices in counter-clockwise order.
        mesh.points = {
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
        mesh.normals = {
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
        mesh.texCoords = {
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
        mesh.indices = {
            0, 1, 2,  0, 2, 3,  // Front face
            4, 5, 6,  4, 6, 7,  // Right face
            8, 9, 10, 8, 10,11, // Back face
            12,13,14, 12,14,15, // Left face
            16,17,18, 16,18,19, // Bottom face
            20,21,22, 20,22,23  // Top face
        };

    }


    // Generates a sphere mesh with specified parameters.
    // The sphere is constructed as a mesh of triangles.
    inline void generateSphere(MeshData& mesh,     // Output mesh.
        float radius = 1.0f,           // Radius of the sphere.
        GLuint nSlices = 20,           // Number of slices (longitudinal divisions).
        GLuint nStacks = 20)           // Number of stacks (latitudinal divisions).
    {
        // Calculate the total number of vertices and faces.
        const int nVerts = (nSlices + 1) * (nStacks + 1);    // Total number of vertices (include duplicate for seamless wrapping).
        const int nFaces = (nSlices * 2 * (nStacks - 1)) * 3; // Total number of face indices (triangles).

        // Resize the output vectors to hold the required data.
        mesh.points.resize(3 * nVerts);                           // Each vertex has 3 coordinates (x, y, z).
        mesh.normals.resize(3 * nVerts);                          // Each vertex has 3 normal components (x, y, z).
        mesh.texCoords.resize(2 * nVerts);                        // Each vertex has 2 texture coordinates (u, v).
        mesh.indices.resize(nFaces);                              // Total number of indices for the faces.

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
                mesh.points[idx] = radius * nx;                   // x-coordinate.
                mesh.points[idx + 1] = radius * ny;               // y-coordinate.
                mesh.points[idx + 2] = radius * nz;               // z-coordinate.

                // Store the normalized normal vector.
                mesh.normals[idx] = nx;                           // Normal x-component.
                mesh.normals[idx + 1] = ny;                       // Normal y-component.
                mesh.normals[idx + 2] = nz;                       // Normal z-component.

                idx += 3;                                    // Advance vertex index.

                // Store texture coordinates.
                mesh.texCoords[tIdx] = s;                         // u-coordinate.
                mesh.texCoords[tIdx + 1] = t;                     // v-coordinate.

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
                    mesh.indices[idx] = stackStart;               // Vertex at the pole.
                    mesh.indices[idx + 1] = stackStart + 1;       // First vertex of the triangle.
                    mesh.indices[idx + 2] = nextStackStart + 1;   // Second vertex of the triangle.
                    idx += 3;                                // Advance index count.
                }
                // Handle the last stack (triangles near the pole).
                else if (j == nStacks - 1) {
                    mesh.indices[idx] = stackStart + j;           // First vertex of the triangle.
                    mesh.indices[idx + 1] = stackStart + j + 1;   // Second vertex of the triangle.
                    mesh.indices[idx + 2] = nextStackStart + j;   // Vertex at the pole.
                    idx += 3;                                // Advance index count.
                }
                // Handle the middle stacks (quads split into two triangles).
                else {
                    // First triangle of the quad.
                    mesh.indices[idx] = stackStart + j;
                    mesh.indices[idx + 1] = stackStart + j + 1;
                    mesh.indices[idx + 2] = nextStackStart + j + 1;

                    // Second triangle of the quad.
                    mesh.indices[idx + 3] = nextStackStart + j;
                    mesh.indices[idx + 4] = stackStart + j;
                    mesh.indices[idx + 5] = nextStackStart + j + 1;

                    idx += 6;                                // Advance index count.
                }
            }
        }

        // Note: Tangents are not calculated here, but the vector is prepared to be populated if needed.
    }


    // Generates a cube-shaped skybox with specified side length.
    // The skybox is constructed as a set of 6 faces (quads) and rendered using triangles.
    inline void generateSkyBox(MeshData& mesh,     // Output mesh
        GLfloat side = 50.0f)           // Length of a side of the cube.
    {
        // Half the length of the side, used to center the cube at the origin.
        const GLfloat side2 = side / 2.0f;

        // Define the vertex positions for the 6 faces of the cube.
        // Each face is defined as a quad made of 4 vertices.
        mesh.points = {
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
        mesh.normals.resize(mesh.points.size(), 0.0f);

        // Define the indices for rendering each face of the cube using two triangles.
        // Each face consists of 6 indices (2 triangles per face).
        mesh.indices = {
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
    }

    //void generate(const char* fileName,
    //    std::vector<GLuint>& indices,
    //    std::vector<GLfloat>& points,
    //    std::vector<GLfloat>& normals,
    //    std::vector<GLfloat>& texCoords,
    //    std::vector<GLfloat>& tangents,
    //    bool genTangents /*= false*/)
    //{

    //    struct MeshData {
    //        std::vector <glm::vec3> points;
    //        std::vector <glm::vec3> normals;
    //        std::vector <glm::vec2> texCoords;
    //        std::vector <Vertex> faces;
    //        std::vector <glm::vec4> tangents;
    //    } meshData;

    //    // Load data
    //    std::ifstream objStream(fileName, std::ios::in);

    //    if (!objStream)
    //        throw std::runtime_error(std::string("file ") + fileName + " not found.");

    //    std::string line, token;
    //    getline(objStream, line);
    //    while (!objStream.eof())
    //    {
    //        // Remove comment if it exists
    //        size_t pos = line.find_first_of("#");
    //        if (pos != std::string::npos)
    //            line = line.substr(0, pos);

    //        // Trim string
    //        const char* whiteSpace = " \t\n\r";
    //        size_t location;
    //        location = line.find_first_not_of(whiteSpace);
    //        line.erase(0, location);
    //        location = line.find_last_not_of(whiteSpace);
    //        line.erase(location + 1);

    //        if (line.length() > 0) {
    //            std::istringstream lineStream(line);

    //            lineStream >> token;

    //            if (token == "v") {
    //                float x, y, z;
    //                lineStream >> x >> y >> z;
    //                glm::vec3 p(x, y, z);
    //                meshData.points.push_back(p);
    //            }
    //            else if (token == "vt") {
    //                // Process texture coordinate
    //                float s, t;
    //                lineStream >> s >> t;
    //                meshData.texCoords.push_back(glm::vec2(s, t));
    //            }
    //            else if (token == "vn") {
    //                float x, y, z;
    //                lineStream >> x >> y >> z;
    //                meshData.normals.push_back(glm::vec3(x, y, z));
    //            }
    //            else if (token == "f") {
    //                std::vector<std::string> parts;
    //                while (lineStream.good()) {
    //                    std::string s;
    //                    lineStream >> s;
    //                    parts.push_back(s);
    //                }

    //                // Triangulate as a triangle fan
    //                if (parts.size() > 2) {
    //                    Vertex firstVert(parts[0]);
    //                    for (int i = 2; i < parts.size(); i++) {
    //                        meshData.faces.push_back(firstVert);
    //                        meshData.faces.push_back(Vertex(parts[i - 1]));
    //                        meshData.faces.push_back(Vertex(parts[i]));
    //                    }
    //                }
    //            }
    //        }
    //        getline(objStream, line);
    //    }
    //    objStream.close();

    //    // Generate normals if needed
    //    if (meshData.normals.size() == 0)
    //    {
    //        meshData.normals.resize(meshData.points.size());

    //        for (GLuint i = 0; i < meshData.faces.size(); i += 3)
    //        {
    //            const glm::vec3& p1 = meshData.points[meshData.faces[i].pIdx];
    //            const glm::vec3& p2 = meshData.points[meshData.faces[i + 1].pIdx];
    //            const glm::vec3& p3 = meshData.points[meshData.faces[i + 2].pIdx];

    //            glm::vec3 a = p2 - p1;
    //            glm::vec3 b = p3 - p1;
    //            glm::vec3 n = glm::normalize(glm::cross(a, b));

    //            meshData.normals[meshData.faces[i].pIdx] += n;
    //            meshData.normals[meshData.faces[i + 1].pIdx] += n;
    //            meshData.normals[meshData.faces[i + 2].pIdx] += n;

    //            // Set the normal index to be the same as the point index
    //            meshData.faces[i].nIdx = meshData.faces[i].pIdx;
    //            meshData.faces[i + 1].nIdx = meshData.faces[i + 1].pIdx;
    //            meshData.faces[i + 2].nIdx = meshData.faces[i + 2].pIdx;
    //        }

    //        for (GLuint i = 0; i < meshData.normals.size(); i++)
    //            meshData.normals[i] = glm::normalize(meshData.normals[i]);
    //    }

    //    // Generate tangents if needed
    //    if (genTangents)
    //    {
    //        std::vector<glm::vec3> tan1Accum(meshData.points.size());
    //        std::vector<glm::vec3> tan2Accum(meshData.points.size());
    //        meshData.tangents.resize(meshData.points.size());

    //        // Compute the tangent std::vector
    //        for (GLuint i = 0; i < meshData.faces.size(); i += 3)
    //        {
    //            const glm::vec3& p1 = meshData.points[meshData.faces[i].pIdx];
    //            const glm::vec3& p2 = meshData.points[meshData.faces[i + 1].pIdx];
    //            const glm::vec3& p3 = meshData.points[meshData.faces[i + 2].pIdx];

    //            const glm::vec2& tc1 = meshData.texCoords[meshData.faces[i].tcIdx];
    //            const glm::vec2& tc2 = meshData.texCoords[meshData.faces[i + 1].tcIdx];
    //            const glm::vec2& tc3 = meshData.texCoords[meshData.faces[i + 2].tcIdx];

    //            glm::vec3 q1 = p2 - p1;
    //            glm::vec3 q2 = p3 - p1;
    //            float s1 = tc2.x - tc1.x, s2 = tc3.x - tc1.x;
    //            float t1 = tc2.y - tc1.y, t2 = tc3.y - tc1.y;
    //            float r = 1.0f / (s1 * t2 - s2 * t1);
    //            glm::vec3 tan1((t2 * q1.x - t1 * q2.x) * r,
    //                (t2 * q1.y - t1 * q2.y) * r,
    //                (t2 * q1.z - t1 * q2.z) * r);
    //            glm::vec3 tan2((s1 * q2.x - s2 * q1.x) * r,
    //                (s1 * q2.y - s2 * q1.y) * r,
    //                (s1 * q2.z - s2 * q1.z) * r);
    //            tan1Accum[meshData.faces[i].pIdx] += tan1;
    //            tan1Accum[meshData.faces[i + 1].pIdx] += tan1;
    //            tan1Accum[meshData.faces[i + 2].pIdx] += tan1;
    //            tan2Accum[meshData.faces[i].pIdx] += tan2;
    //            tan2Accum[meshData.faces[i + 1].pIdx] += tan2;
    //            tan2Accum[meshData.faces[i + 2].pIdx] += tan2;
    //        }

    //        for (GLuint i = 0; i < meshData.points.size(); ++i)
    //        {
    //            const glm::vec3& n = meshData.normals[i];
    //            glm::vec3& t1 = tan1Accum[i];
    //            glm::vec3& t2 = tan2Accum[i];

    //            // Gram-Schmidt orthogonalize
    //            meshData.tangents[i] = glm::vec4(glm::normalize(t1 - (glm::dot(n, t1) * n)), 0.0f);
    //            // Store handedness in w
    //            meshData.tangents[i].w = (glm::dot(glm::cross(n, t1), t2) < 0.0f) ? -1.0f : 1.0f;
    //        }
    //    }

    //    // Convert to GL format
    //    std::map<std::string, GLuint> vertexMap;
    //    for (auto& vert : meshData.faces)
    //    {
    //        std::string vertStr = std::to_string(vert.pIdx) + "/" +
    //            std::to_string(vert.tcIdx) + "/" +
    //            std::to_string(vert.nIdx);
    //        auto it = vertexMap.find(vertStr);
    //        if (it == vertexMap.end()) {
    //            auto vIdx = points.size() / 3;

    //            auto& pt = meshData.points[vert.pIdx];
    //            points.push_back(pt.x);
    //            points.push_back(pt.y);
    //            points.push_back(pt.z);

    //            auto& n = meshData.normals[vert.nIdx];
    //            normals.push_back(n.x);
    //            normals.push_back(n.y);
    //            normals.push_back(n.z);

    //            if (!meshData.texCoords.empty()) {
    //                auto& tc = meshData.texCoords[vert.tcIdx];
    //                texCoords.push_back(tc.x);
    //                texCoords.push_back(tc.y);
    //            }

    //            if (!meshData.tangents.empty()) {
    //                // We use the point index for tangents
    //                auto& tang = meshData.tangents[vert.pIdx];
    //                tangents.push_back(tang.x);
    //                tangents.push_back(tang.y);
    //                tangents.push_back(tang.z);
    //                tangents.push_back(tang.w);
    //            }

    //            indices.push_back((GLuint)vIdx);
    //            vertexMap[vertStr] = (GLuint)vIdx;
    //        }
    //        else
    //            indices.push_back(it->second);
    //    }

    //}

}

#endif // MESHGENERATOR_H