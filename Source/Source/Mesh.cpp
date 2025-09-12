#include "pch.hpp"
#include "global.hpp"
#include "Mesh.h"


void Mesh::PushQuad(std::vector<glm::ivec3>& Tri, int i, int j, int k, int l)
{
    Tri.push_back(glm::ivec3(i, j, k));
    Tri.push_back(glm::ivec3(i, k, l));
}

// Implementation
std::shared_ptr<Mesh> Mesh::CreateSphere(int n) {
    std::cout << "Mesh::CreateSphere starting with n=" << n << std::endl;
    
    auto mesh = std::make_shared<Mesh>();

    // Your existing sphere generation code here
    for (int i = 0; i <= n * 2; i++) {
        float s = i * 2.0f * PI / float(n * 2);
        for (int j = 0; j <= n; j++) {
            float t = j * PI / float(n);
            float x = cos(s) * sin(t);
            float y = sin(s) * sin(t);
            float z = cos(t);
            mesh->vertices.push_back(glm::vec4(x, y, z, 1.0f));
            mesh->normals.push_back(glm::vec3(x, y, z));
            mesh->texCoords.push_back(glm::vec2(s / (2 * PI), t / PI));
            mesh->tangents.push_back(glm::vec3(-sin(s), cos(s), 0.0));
            // Triangle generation...
            if (i > 0 && j > 0) {
                mesh->PushQuad(mesh->triangles, (i - 1) * (n + 1) + (j - 1),
                    (i - 1) * (n + 1) + (j),
                    (i) * (n + 1) + (j),
                    (i) * (n + 1) + (j - 1));
            }
        }
    }

    std::cout << "Sphere generation completed:" << std::endl;
    std::cout << "  - Vertices: " << mesh->vertices.size() << std::endl;
    std::cout << "  - Normals: " << mesh->normals.size() << std::endl;
    std::cout << "  - TexCoords: " << mesh->texCoords.size() << std::endl;
    std::cout << "  - Triangles: " << mesh->triangles.size() << std::endl;

    mesh->MakeVAO();
    std::cout << "VAO created with ID: " << mesh->vaoID << std::endl;
    
    return mesh;
}

std::shared_ptr<Mesh> Mesh::CreatePlane(float size, int subdivisions) {
    auto mesh = std::make_shared<Mesh>();
    float halfSize = size / 2.0f;
    int n = subdivisions;
    for (int i = 0; i <= n; i++) {
        float s = i / float(n);
        for (int j = 0; j <= n; j++) {
            float t = j / float(n);
            float x = s * size - halfSize;
            float y = t * size - halfSize;
            float z = 0.0f;
            mesh->vertices.push_back(glm::vec4(x, y, z, 1.0f));
            mesh->normals.push_back(glm::vec3(0.0f, 0.0f, 1.0f));
            mesh->texCoords.push_back(glm::vec2(s, t));
            mesh->tangents.push_back(glm::vec3(1.0, 0.0, 0.0));
            if (i > 0 && j > 0) {
                mesh->PushQuad(mesh->triangles,
                    (i - 1) * (n + 1) + (j - 1),
                    (i - 1) * (n + 1) + (j),
                    (i) * (n + 1) + (j),
                    (i) * (n + 1) + (j - 1));
            }
        }
    }
    mesh->MakeVAO();
	return mesh;
}

std::shared_ptr<Mesh> Mesh::LoadFromFile(const std::string& path)
{
    // Implementation for loading a mesh from a file (e.g., OBJ, FBX) would go here.
    // For now, it can be a placeholder.
	UNREFERENCED_PARAMETER(path);
    return std::make_shared<Mesh>();
}


void Mesh::MakeVAO() {
    std::cout << "Mesh::MakeVAO starting..." << std::endl;
    
    glGenVertexArrays(1, &vaoID);
    glBindVertexArray(vaoID);
    
    std::cout << "VAO generated and bound: " << vaoID << std::endl;

    GLuint Pbuff;
    glGenBuffers(1, &Pbuff);
    glBindBuffer(GL_ARRAY_BUFFER, Pbuff);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec4) * vertices.size(),
        vertices.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);
    std::cout << "Vertex buffer created with " << vertices.size() << " vertices" << std::endl;

    if (normals.size() > 0) {
        GLuint Nbuff;
        glGenBuffers(1, &Nbuff);
        glBindBuffer(GL_ARRAY_BUFFER, Nbuff);
        glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * normals.size(),
            normals.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
        std::cout << "Normal buffer created with " << normals.size() << " normals" << std::endl;
    }

    if (texCoords.size() > 0) {
        GLuint Tbuff;
        glGenBuffers(1, &Tbuff);
        glBindBuffer(GL_ARRAY_BUFFER, Tbuff);
        glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec2) * texCoords.size(),
            texCoords.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);
        std::cout << "TexCoord buffer created with " << texCoords.size() << " coords" << std::endl;
    }

    if (tangents.size() > 0) {
        GLuint Dbuff;
        glGenBuffers(1, &Dbuff);
        glBindBuffer(GL_ARRAY_BUFFER, Dbuff);
        glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * tangents.size(),
            tangents.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, 0);
        std::cout << "Tangent buffer created with " << tangents.size() << " tangents" << std::endl;
    }

    GLuint Ibuff;
    glGenBuffers(1, &Ibuff);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Ibuff);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(glm::ivec3) * triangles.size(),
        triangles.data(), GL_STATIC_DRAW);
    std::cout << "Index buffer created with " << triangles.size() << " triangles" << std::endl;

    glBindVertexArray(0);
    std::cout << "VAO unbound, MakeVAO completed" << std::endl;
    
    // Check for OpenGL errors
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cout << "OpenGL Error in MakeVAO: " << error << std::endl;
    }
}

void Mesh::Draw() {
    static int meshDrawCount = 0;
    meshDrawCount++;
    
    // Only debug every 60 calls to avoid spam
    bool shouldDebug = (meshDrawCount % 60 == 1);
    
    if (shouldDebug) {
        std::cout << "Mesh::Draw called (call #" << meshDrawCount << ")" << std::endl;
        std::cout << "  - VAO ID: " << vaoID << std::endl;
        std::cout << "  - Triangle count: " << triangles.size() << std::endl;
    }
    
    if (triangles.empty()) {
        if (shouldDebug) {
            std::cout << "  - No triangles to draw!" << std::endl;
        }
        return;
    }
    
    glBindVertexArray(vaoID);
    if (shouldDebug) {
        std::cout << "  - VAO bound, calling glDrawElements" << std::endl;
    }
    
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(triangles.size() * 3), GL_UNSIGNED_INT, 0);
    
    if (shouldDebug) {
        std::cout << "  - glDrawElements completed" << std::endl;
    }
    
    glBindVertexArray(0);
    
    // Check for OpenGL errors
    GLenum error = glGetError();
    if (error != GL_NO_ERROR && shouldDebug) {
        std::cout << "  - OpenGL Error in Mesh::Draw: " << error << std::endl;
    }
}