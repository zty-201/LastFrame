#include "pch.hpp"
#include "Mesh.h"

void Mesh::PushQuad(std::vector<glm::ivec3>& Tri, int i, int j, int k, int l)
{
    Tri.push_back(glm::ivec3(i, j, k));
    Tri.push_back(glm::ivec3(i, k, l));
}

// Implementation
std::shared_ptr<Mesh> Mesh::CreateSphere(int n) {
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


    mesh->MakeVAO();
    return mesh;
}

void Mesh::MakeVAO() {
    glGenVertexArrays(1, &vaoID);
    glBindVertexArray(vaoID);

    GLuint Pbuff;
    glGenBuffers(1, &Pbuff);
    glBindBuffer(GL_ARRAY_BUFFER, Pbuff);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 4 * vertices.size(),
        &vertices[0][0], GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    if (normals.size() > 0) {
        GLuint Nbuff;
        glGenBuffers(1, &Nbuff);
        glBindBuffer(GL_ARRAY_BUFFER, Nbuff);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 3 * normals.size(),
            &normals[0][0], GL_STATIC_DRAW);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    if (texCoords.size() > 0) {
        GLuint Tbuff;
        glGenBuffers(1, &Tbuff);
        glBindBuffer(GL_ARRAY_BUFFER, Tbuff);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 2 * texCoords.size(),
            &texCoords[0][0], GL_STATIC_DRAW);
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    if (tangents.size() > 0) {
        GLuint Dbuff;
        glGenBuffers(1, &Dbuff);
        glBindBuffer(GL_ARRAY_BUFFER, Dbuff);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 3 * tangents.size(),
            &tangents[0][0], GL_STATIC_DRAW);
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    GLuint Ibuff;
    glGenBuffers(1, &Ibuff);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Ibuff);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(int) * 3 * triangles.size(),
        &triangles[0][0], GL_STATIC_DRAW);

    glBindVertexArray(0);
}