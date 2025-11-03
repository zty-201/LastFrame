/**
 * @file Mesh.h
 * @brief Provides utilities for generating and managing 3D mesh data.
 *
 * This file defines the `Mesh` namespace, which includes structures and functions
 * for creating, parsing, and manipulating 3D mesh data. It supports loading meshes
 * from files using Assimp, generating common 3D primitives (e.g., quads, cubes, spheres),
 * and calculating tangents for advanced rendering techniques.
 *
 * Key components:
 * - `Mesh::Vertex`: Represents a vertex with position, normal, UVs, and tangent.
 * - `Mesh::MeshData`: Stores vertex data, sub-mesh info, materials, and OpenGL handles.
 * - Utility functions for generating 3D primitives and calculating tangents.
 *
 * @author zty-201
 * @date 2025-11-03
 */
#ifndef MESH_H
#define MESH_H

#include "nlohmann/json.hpp"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "MaterialComponent.h"
#include "global.hpp" // For AssetManagerSystem, etc.

namespace Mesh {

    // Represents a single drawable part (mesh group) of a larger model.
    struct SubMesh {
        std::string name;
        int materialIndex = -1; // Index into the parent MeshData's materials vector.
        GLuint indexCount = 0;
        GLuint baseIndex = 0;
        GLuint baseVertex = 0;
    };

    // A flexible vertex struct for loading any kind of mesh.
    struct Vertex {
        glm::vec3 Position;
        glm::vec3 Normal;
        glm::vec2 TexCoords;
        glm::vec4 Tangent; // Using vec4 to include handedness (w component)
    };

    // The main MeshData class, handling both loaded and generated meshes.
    class MeshData {
    public:
        // OpenGL handles for the entire model's geometry.
        GLuint vao = 0;
        GLuint vertexBuffer = 0;
        GLuint indexBuffer = 0;

        // A mesh is a collection of sub-meshes and their materials.
        std::vector<SubMesh> subMeshes;
        std::vector<MaterialComponent> materials;

        // Default constructor.
        MeshData() = default;

        // Constructor for loading a mesh from a file via Assimp.
        explicit MeshData(const std::string& fileName) {
            loadWithAssimp(fileName);
        }

        // Constructor for manually creating a mesh from code (for primitives).
        explicit MeshData(const std::vector<Vertex>& vertices, const std::vector<GLuint>& indices) {
            SubMesh primitiveSubMesh;
            primitiveSubMesh.name = "default";
            primitiveSubMesh.materialIndex = 0;
            primitiveSubMesh.baseIndex = 0;
            primitiveSubMesh.baseVertex = 0;
            primitiveSubMesh.indexCount = indices.size();
            subMeshes.push_back(primitiveSubMesh);

            MaterialComponent defaultMaterial;
            defaultMaterial.name = "default_material";
            materials.push_back(defaultMaterial);

            createGLBuffers(vertices, indices);
        }

    private:
        // The core Assimp loading function.
        void loadWithAssimp(const std::string& fileName) {
            Assimp::Importer importer;
            const aiScene* scene = importer.ReadFile(fileName,
                aiProcess_Triangulate |
                aiProcess_GenSmoothNormals |
                aiProcess_CalcTangentSpace | // Ask Assimp to calculate tangents
                aiProcess_FlipUVs);

            if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
                throw std::runtime_error("Assimp Error: " + std::string(importer.GetErrorString()));
            }

            processMaterials(scene);
            processNode(scene->mRootNode, scene);
            createGLBuffers(m_temp_vertices, m_temp_indices);

            m_temp_vertices.clear();
            m_temp_indices.clear();
        }

        // Extracts materials from the Assimp scene.
        void processMaterials(const aiScene* scene) {
            for (unsigned int i = 0; i < scene->mNumMaterials; i++) {
                aiMaterial* mat = scene->mMaterials[i];
                MaterialComponent newMat;
                newMat.name = mat->GetName().C_Str();
                aiString path_str;

                if (mat->GetTexture(aiTextureType_DIFFUSE, 0, &path_str) == AI_SUCCESS) {
                    std::filesystem::path texture_path(path_str.C_Str());
                    newMat.albedoMap = EngineFramework::AssetManagerSystem::GetFullGuid(EngineFramework::AssetManagerSystem::TEXTURE_GUID, texture_path.stem().string());
                }
                if (mat->GetTexture(aiTextureType_NORMALS, 0, &path_str) == AI_SUCCESS) {
                    std::filesystem::path texture_path(path_str.C_Str());
                    newMat.normalMap = EngineFramework::AssetManagerSystem::GetFullGuid(EngineFramework::AssetManagerSystem::TEXTURE_GUID, texture_path.stem().string());
                }
                // Add other texture types as needed (metallic, roughness, etc.)

                materials.push_back(newMat);
            }
        }

        // Recursively processes Assimp nodes.
        void processNode(aiNode* node, const aiScene* scene) {
            for (unsigned int i = 0; i < node->mNumMeshes; i++) {
                processSubMesh(scene->mMeshes[node->mMeshes[i]], scene);
            }
            for (unsigned int i = 0; i < node->mNumChildren; i++) {
                processNode(node->mChildren[i], scene);
            }
        }

        // Extracts vertex data from a single Assimp mesh.
        void processSubMesh(aiMesh* mesh, const aiScene* scene) {
            SubMesh subMesh;
            subMesh.name = mesh->mName.C_Str();
            subMesh.materialIndex = mesh->mMaterialIndex;
            subMesh.baseVertex = m_temp_vertices.size();
            subMesh.baseIndex = m_temp_indices.size();
            subMesh.indexCount = 0;

            for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
                Vertex vertex{};
                vertex.Position = { mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z };
                vertex.Normal = { mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z };
                if (mesh->mTextureCoords[0]) {
                    vertex.TexCoords = { mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y };
                }
                if (mesh->mTangents) {
                    // Assimp provides the tangent and bitangent, we use them to calculate the w component
                    glm::vec3 t = { mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z };
                    glm::vec3 b = { mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z };
                    float handedness = (glm::dot(glm::cross(vertex.Normal, t), b) < 0.0f) ? -1.0f : 1.0f;
                    vertex.Tangent = glm::vec4(t, handedness);
                }
                m_temp_vertices.push_back(vertex);
            }

            for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
                aiFace face = mesh->mFaces[i];
                for (unsigned int j = 0; j < face.mNumIndices; j++) {
                    m_temp_indices.push_back(subMesh.baseVertex + face.mIndices[j]);
                }
                subMesh.indexCount += face.mNumIndices;
            }
            subMeshes.push_back(subMesh);
        }

        // Creates the final OpenGL VAO and buffers.
        void createGLBuffers(const std::vector<Vertex>& vertices, const std::vector<GLuint>& indices) {
            glGenVertexArrays(1, &vao);
            glBindVertexArray(vao);

            glGenBuffers(1, &vertexBuffer);
            glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
            glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

            glGenBuffers(1, &indexBuffer);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);

            glEnableVertexAttribArray(0); // Position
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Position));
            glEnableVertexAttribArray(1); // Normal
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
            glEnableVertexAttribArray(2); // TexCoords
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));
            glEnableVertexAttribArray(3); // Tangent (now a vec4)
            glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Tangent));

            glBindVertexArray(0);
        }

    private:
        std::vector<Vertex> m_temp_vertices;
        std::vector<GLuint> m_temp_indices;
    };

    // =========================================================================
    // Functions to generate primitive meshes from code    
    // =========================================================================

    // Generic helper function to calculate tangents for any manually created mesh
    inline void calculateTangents(std::vector<Vertex>& vertices, const std::vector<GLuint>& indices) {
        if (indices.empty() || vertices.empty()) return;

        std::vector<glm::vec3> tan1(vertices.size(), glm::vec3(0.0f));
        std::vector<glm::vec3> tan2(vertices.size(), glm::vec3(0.0f));

        for (size_t i = 0; i < indices.size(); i += 3) {
            Vertex& v0 = vertices[indices[i]];
            Vertex& v1 = vertices[indices[i + 1]];
            Vertex& v2 = vertices[indices[i + 2]];
            glm::vec3 edge1 = v1.Position - v0.Position;
            glm::vec3 edge2 = v2.Position - v0.Position;
            glm::vec2 deltaUV1 = v1.TexCoords - v0.TexCoords;
            glm::vec2 deltaUV2 = v2.TexCoords - v0.TexCoords;
            float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);
            glm::vec3 tangent, bitangent;
            tangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
            tangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
            tangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
            bitangent.x = f * (-deltaUV1.x * edge2.x + deltaUV2.x * edge1.x);
            bitangent.y = f * (-deltaUV1.x * edge2.y + deltaUV2.x * edge1.y);
            bitangent.z = f * (-deltaUV1.x * edge2.z + deltaUV2.x * edge1.z);
            tan1[indices[i]] += tangent;
            tan1[indices[i + 1]] += tangent;
            tan1[indices[i + 2]] += tangent;
            tan2[indices[i]] += bitangent;
            tan2[indices[i + 1]] += bitangent;
            tan2[indices[i + 2]] += bitangent;
        }

        for (size_t i = 0; i < vertices.size(); ++i) {
            const glm::vec3& n = vertices[i].Normal;
            const glm::vec3& t = tan1[i];
            glm::vec3 tangent = glm::normalize(t - n * glm::dot(n, t));
            float handedness = (glm::dot(glm::cross(n, t), tan2[i]) < 0.0f) ? -1.0f : 1.0f;
            vertices[i].Tangent = glm::vec4(tangent, handedness);
        }
    }

    // Generates a standard cube
    inline std::shared_ptr<MeshData> generateCube() {
        std::vector<Vertex> vertices = {
            // Positions, Normals, UVs
            {{-0.5f, -0.5f, -0.5f}, {0.0f,  0.0f, -1.0f}, {0.0f, 0.0f}}, {{0.5f,  0.5f, -0.5f}, {0.0f,  0.0f, -1.0f}, {1.0f, 1.0f}}, {{0.5f, -0.5f, -0.5f}, {0.0f,  0.0f, -1.0f}, {1.0f, 0.0f}}, {{-0.5f,  0.5f, -0.5f}, {0.0f,  0.0f, -1.0f}, {0.0f, 1.0f}},
            {{-0.5f, -0.5f,  0.5f}, {0.0f,  0.0f,  1.0f}, {0.0f, 0.0f}}, {{0.5f, -0.5f,  0.5f}, {0.0f,  0.0f,  1.0f}, {1.0f, 0.0f}}, {{0.5f,  0.5f,  0.5f}, {0.0f,  0.0f,  1.0f}, {1.0f, 1.0f}}, {{-0.5f,  0.5f,  0.5f}, {0.0f,  0.0f,  1.0f}, {0.0f, 1.0f}},
            {{-0.5f,  0.5f,  0.5f}, {-1.0f,  0.0f,  0.0f}, {1.0f, 0.0f}}, {{-0.5f,  0.5f, -0.5f}, {-1.0f,  0.0f,  0.0f}, {1.0f, 1.0f}}, {{-0.5f, -0.5f, -0.5f}, {-1.0f,  0.0f,  0.0f}, {0.0f, 1.0f}}, {{-0.5f, -0.5f,  0.5f}, {-1.0f,  0.0f,  0.0f}, {0.0f, 0.0f}},
            {{0.5f,  0.5f,  0.5f}, {1.0f,  0.0f,  0.0f}, {1.0f, 0.0f}}, {{0.5f, -0.5f, -0.5f}, {1.0f,  0.0f,  0.0f}, {0.0f, 1.0f}}, {{0.5f,  0.5f, -0.5f}, {1.0f,  0.0f,  0.0f}, {1.0f, 1.0f}}, {{0.5f, -0.5f,  0.5f}, {1.0f,  0.0f,  0.0f}, {0.0f, 0.0f}},
            {{-0.5f, -0.5f, -0.5f}, {0.0f, -1.0f,  0.0f}, {0.0f, 1.0f}}, {{0.5f, -0.5f, -0.5f}, {0.0f, -1.0f,  0.0f}, {1.0f, 1.0f}}, {{0.5f, -0.5f,  0.5f}, {0.0f, -1.0f,  0.0f}, {1.0f, 0.0f}}, {{-0.5f, -0.5f,  0.5f}, {0.0f, -1.0f,  0.0f}, {0.0f, 0.0f}},
            {{-0.5f,  0.5f, -0.5f}, {0.0f,  1.0f,  0.0f}, {0.0f, 1.0f}}, {{0.5f,  0.5f,  0.5f}, {0.0f,  1.0f,  0.0f}, {1.0f, 0.0f}}, {{0.5f,  0.5f, -0.5f}, {0.0f,  1.0f,  0.0f}, {1.0f, 1.0f}}, {{-0.5f,  0.5f,  0.5f}, {0.0f,  1.0f,  0.0f}, {0.0f, 0.0f}}
        };
        std::vector<GLuint> indices = { 0, 1, 2, 0, 3, 1, 4, 5, 6, 4, 6, 7, 8, 9, 10, 8, 10, 11, 12, 13, 14, 12, 15, 13, 16, 17, 18, 16, 18, 19, 20, 21, 22, 20, 23, 21 };
        calculateTangents(vertices, indices);
        return std::make_shared<MeshData>(vertices, indices);
    }

    // Generates a UV sphere
    inline std::shared_ptr<MeshData> generateSphere(int xSegments = 64, int ySegments = 64) {
        std::vector<Vertex> vertices;
        std::vector<GLuint> indices;
        for (int y = 0; y <= ySegments; ++y) {
            for (int x = 0; x <= xSegments; ++x) {
                float xSegment = (float)x / (float)xSegments;
                float ySegment = (float)y / (float)ySegments;
                float xPos = std::cos(xSegment * 2.0f * PI) * std::sin(ySegment * PI);
                float yPos = std::cos(ySegment * PI);
                float zPos = std::sin(xSegment * 2.0f * PI) * std::sin(ySegment * PI);
                vertices.push_back({ {xPos, yPos, zPos}, {xPos, yPos, zPos}, {xSegment, ySegment} });
            }
        }
        for (int y = 0; y < ySegments; ++y) {
            for (int x = 0; x < xSegments; ++x) {
                indices.push_back((y + 1) * (xSegments + 1) + x);
                indices.push_back(y * (xSegments + 1) + x);
                indices.push_back(y * (xSegments + 1) + x + 1);
                indices.push_back((y + 1) * (xSegments + 1) + x);
                indices.push_back(y * (xSegments + 1) + x + 1);
                indices.push_back((y + 1) * (xSegments + 1) + x + 1);
            }
        }
        calculateTangents(vertices, indices);
        return std::make_shared<MeshData>(vertices, indices);
    }

    // Generates a simple quad
    inline std::shared_ptr<MeshData> generateQuad() {
        std::vector<Vertex> vertices = {
            {{-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
            {{ 1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
            {{ 1.0f,  1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
            {{-1.0f,  1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}}
        };
        std::vector<GLuint> indices = { 0, 1, 2, 2, 3, 0 };
        calculateTangents(vertices, indices);
        return std::make_shared<MeshData>(vertices, indices);
    }

    // Generates a cube for skybox rendering (no normals, UVs, or tangents needed)
    inline std::shared_ptr<MeshData> generateSkyBox() {
        std::vector<Vertex> vertices = {
            {{-1.0f,  1.0f, -1.0f}}, {{-1.0f, -1.0f, -1.0f}}, {{1.0f, -1.0f, -1.0f}}, {{1.0f,  1.0f, -1.0f}},
            {{-1.0f, -1.0f,  1.0f}}, {{-1.0f,  1.0f,  1.0f}}, {{1.0f, -1.0f,  1.0f}}, {{1.0f,  1.0f,  1.0f}}
        };
        std::vector<GLuint> indices = { 0, 1, 2, 0, 2, 3, 1, 4, 6, 1, 6, 2, 4, 5, 7, 4, 7, 6, 5, 0, 3, 5, 3, 7, 3, 2, 6, 3, 6, 7, 0, 5, 4, 0, 4, 1 };
        return std::make_shared<MeshData>(vertices, indices);
    }

} // namespace Mesh

#endif // MESH_H