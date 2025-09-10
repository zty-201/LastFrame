#ifndef MESH_H
#define MESH_H

class Mesh {
private:
    std::vector<glm::vec4> vertices;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> texCoords;
    std::vector<glm::vec3> tangents;
    std::vector<glm::ivec3> triangles;

    GLuint vaoID;
    unsigned int count;

public:
    // Factory functions for procedural geometry
    static std::shared_ptr<Mesh> CreateSphere(int subdivisions);
    static std::shared_ptr<Mesh> CreatePlane(float size, int subdivisions);
    static std::shared_ptr<Mesh> CreateCube(float size);
    static std::shared_ptr<Mesh> CreateCylinder(float radius, float height, int segments);

    // For loading from files
    static std::shared_ptr<Mesh> LoadFromFile(const std::string& path);
    void PushQuad(std::vector<glm::ivec3>& Tri, int i, int j, int k, int l);

    void Draw();
    void MakeVAO();
};



#endif
