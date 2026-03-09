#ifndef GEOMETRY_DATA_BUFFER_H
#define GEOMETRY_DATA_BUFFER_H
#include "Mesh.h"

namespace EngineFramework {

    class GeometryDataBuffer {
    public:
        GeometryDataBuffer() = default;
        ~GeometryDataBuffer();

        // Allocates the giant buffers on the GPU upfront
        void Initialize(uint32_t maxVertices, uint32_t maxIndices);

        // Pushes a single mesh into the buffer and returns {baseVertex, firstIndex}
        std::pair<uint32_t, uint32_t> AppendMesh(const std::vector<Mesh::Vertex>& vertices, const std::vector<Mesh::IndexType>& indices);

        // Binds the global VAO for rendering
        void Bind() const;
        void Unbind() const;

        GLuint GetVAO() const { return m_vao; }

    private:
        GLuint m_vao = 0;
        GLuint m_vbo = 0;
        GLuint m_ibo = 0;

        uint32_t m_currentVertexOffset = 0;
        uint32_t m_currentIndexOffset = 0;

        uint32_t m_maxVertices = 0;
        uint32_t m_maxIndices = 0;
    };

}

#endif