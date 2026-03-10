#include "pch.hpp" 
#include "GeometryDataBuffer.h"

namespace EngineFramework {

    GeometryDataBuffer::~GeometryDataBuffer()
    {
        if (m_vao != 0) {
            glDeleteVertexArrays(1, &m_vao);
            glDeleteBuffers(1, &m_vbo);
            glDeleteBuffers(1, &m_ibo);
        }
    }

    void GeometryDataBuffer::Initialize(uint32_t maxVertices, uint32_t maxIndices)
    {
        m_maxVertices = maxVertices;
        m_maxIndices = maxIndices;
        m_currentVertexOffset = 0;
        m_currentIndexOffset = 0;

        glGenVertexArrays(1, &m_vao);
        glGenBuffers(1, &m_vbo);
        glGenBuffers(1, &m_ibo);

        glBindVertexArray(m_vao);

        // 1. Allocate VBO (using GL_STATIC_DRAW since we append once at load time)
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferData(GL_ARRAY_BUFFER, m_maxVertices * sizeof(Mesh::Vertex), nullptr, GL_STATIC_DRAW);

        // 2. Allocate IBO
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_maxIndices * sizeof(Mesh::IndexType), nullptr, GL_STATIC_DRAW);

        // 3. Setup Vertex Attributes (Copied exactly from your old MeshData)

        // 0. Position (12 bytes)
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Mesh::Vertex), (void*)offsetof(Mesh::Vertex, Position));

        // 1. TexCoords (4 bytes) - Packed as 16-bit Half Floats
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_HALF_FLOAT, GL_FALSE, sizeof(Mesh::Vertex), (void*)offsetof(Mesh::Vertex, TexCoords));

        // 2. Normal (4 bytes) - Packed as 10_10_10_2, MUST BE NORMALIZED (GL_TRUE)
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 4, GL_INT_2_10_10_10_REV, GL_TRUE, sizeof(Mesh::Vertex), (void*)offsetof(Mesh::Vertex, Normal));

        // 3. Tangent (4 bytes) - Packed as 10_10_10_2, MUST BE NORMALIZED (GL_TRUE)
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 4, GL_INT_2_10_10_10_REV, GL_TRUE, sizeof(Mesh::Vertex), (void*)offsetof(Mesh::Vertex, Tangent));

        // 4. BoneIndices (4 bytes) - Uses IPointer
        glEnableVertexAttribArray(4);
        glVertexAttribIPointer(4, 4, GL_UNSIGNED_BYTE, sizeof(Mesh::Vertex), (void*)offsetof(Mesh::Vertex, BoneIndices));

        // 5. BoneWeights (4 bytes) - Uses Pointer, MUST BE NORMALIZED (GL_TRUE)
        glEnableVertexAttribArray(5);
        glVertexAttribPointer(5, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Mesh::Vertex), (void*)offsetof(Mesh::Vertex, BoneWeights));

        // Unbind to prevent accidental modification
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        LOGI("GeometryDataBuffer Initialized: Capacity %u Verts, %u Indices", m_maxVertices, m_maxIndices);
    }

    std::pair<uint32_t, uint32_t> GeometryDataBuffer::AppendMesh(const std::vector<Mesh::Vertex>& vertices, const std::vector<Mesh::IndexType>& indices)
    {
        if (vertices.empty() || indices.empty()) {
            return { 0, 0 };
        }

        // Safety check to prevent buffer overflow
        if (m_currentVertexOffset + vertices.size() > m_maxVertices ||
            m_currentIndexOffset + indices.size() > m_maxIndices) {

            LOGE("CRITICAL: GeometryDataBuffer out of memory! Attempted to append %llu verts, %llu indices.", vertices.size(), indices.size());
            // In a production engine, you would allocate a new GeometryDataBuffer chunk here.
            // For now, returning 0,0 prevents a crash, but the mesh won't render correctly.
            return { 0, 0 };
        }

        // Record the starting positions to return to the MeshData
        uint32_t baseVertex = m_currentVertexOffset;
        uint32_t firstIndex = m_currentIndexOffset;

        // Upload Vertices
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferSubData(GL_ARRAY_BUFFER,
            m_currentVertexOffset * sizeof(Mesh::Vertex),
            vertices.size() * sizeof(Mesh::Vertex),
            vertices.data());

        // Upload Indices
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo);
        glBufferSubData(GL_ELEMENT_ARRAY_BUFFER,
            m_currentIndexOffset * sizeof(Mesh::IndexType),
            indices.size() * sizeof(Mesh::IndexType),
            indices.data());

        // Unbind
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        // Bump the internal offsets forward
        m_currentVertexOffset += static_cast<uint32_t>(vertices.size());
        m_currentIndexOffset += static_cast<uint32_t>(indices.size());

        return { baseVertex, firstIndex };
    }

    void GeometryDataBuffer::Bind() const
    {
        glBindVertexArray(m_vao);
    }

    void GeometryDataBuffer::Unbind() const
    {
        glBindVertexArray(0);
    }

} // namespace EngineFramework