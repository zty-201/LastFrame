#ifndef INSTANCINGSYSTEM_H
#define INSTANCINGSYSTEM_H

#include "MaterialComponent.h"
#include "MeshComponent.h"
#include "TransformComponent.h"
#include "AnimationComponent.h"

namespace EngineFramework
{
    // ============================================================================
    // 1. STRICT BATCH KEY (The "Fingerprint")
    // ============================================================================
    struct BatchKey {
        std::string meshName;
        int subMeshIndex;

        // Textures
        xresource::full_guid albedoMap;
        xresource::full_guid normalMap;
        xresource::full_guid roughnessMap;
        xresource::full_guid metallicMap;

        // Factors & UVs
        glm::vec3 albedoFactor;
        float roughnessFactor;
        float metallicFactor;
        glm::vec2 uvTiling;
        glm::vec2 uvOffset;

        bool operator<(const BatchKey& other) const {
            return std::tie(
                meshName, subMeshIndex,
                albedoMap.m_Instance.m_Value, albedoMap.m_Type.m_Value,
                normalMap.m_Instance.m_Value, normalMap.m_Type.m_Value,
                roughnessMap.m_Instance.m_Value, roughnessMap.m_Type.m_Value,
                metallicMap.m_Instance.m_Value, metallicMap.m_Type.m_Value,
                albedoFactor.x, albedoFactor.y, albedoFactor.z,
                roughnessFactor, metallicFactor,
                uvTiling.x, uvTiling.y, uvOffset.x, uvOffset.y
            ) < std::tie(
                other.meshName, other.subMeshIndex,
                other.albedoMap.m_Instance.m_Value, other.albedoMap.m_Type.m_Value,
                other.normalMap.m_Instance.m_Value, other.normalMap.m_Type.m_Value,
                other.roughnessMap.m_Instance.m_Value, other.roughnessMap.m_Type.m_Value,
                other.metallicMap.m_Instance.m_Value, other.metallicMap.m_Type.m_Value,
                other.albedoFactor.x, other.albedoFactor.y, other.albedoFactor.z,
                other.roughnessFactor, other.metallicFactor,
                other.uvTiling.x, other.uvTiling.y, other.uvOffset.x, other.uvOffset.y
            );
        }
    };

    // ============================================================================
    // 2. ULTRA-LEAN INSTANCE DATA
    // ============================================================================
    struct InstanceData {
        glm::mat4 modelMatrix;
        // 64 bytes total. Perfectly aligned for std430.
    };

    // ============================================================================
    // 3. RENDER BATCH
    // ============================================================================
    struct RenderBatch {
        BatchKey key;
        MaterialComponent material; // The shared material state
        size_t instanceOffset;      // Where this batch starts in the global SSBO
        size_t instanceCount;       // How many instances to draw
    };

    // ============================================================================
    // 4. INSTANCING SYSTEM
    // ============================================================================
    class InstancingSystem {
    public:
        InstancingSystem();
        ~InstancingSystem();

        void Initialize(size_t maxTotalInstances = 50000);

        // Gathers all entities, culls them, and builds the batches/SSBO data
        void GatherPass(const glm::mat4& viewProj);

        // Getters for the GraphicsSystem to use during rendering
        const std::vector<RenderBatch>& GetBatches() const { return m_activeBatches; }
        const std::vector<entt::entity>& GetAnimatedEntities() const { return m_animatedEntities; }
        GLuint GetInstanceSSBO() const { return m_instanceSSBO; }

    private:
        GLuint m_instanceSSBO = 0;
        size_t m_maxTotalInstances = 0;

        // Frame-by-frame data
        std::vector<InstanceData> m_globalInstanceData;
        std::vector<RenderBatch> m_activeBatches;
        std::vector<entt::entity> m_animatedEntities;
    };
}
#endif // INSTANCINGSYSTEM_H