#ifndef INSTANCINGSYSTEM_H
#define INSTANCINGSYSTEM_H

#include "MaterialComponent.h"
#include "MeshComponent.h"
#include "TransformComponent.h"
#include "TextureComponent.h"
#include "NormalMapComponent.h"

namespace EngineFramework
{
    // ============================================================================
    // BATCH KEY
    // ============================================================================

    struct BatchKey {
        std::string meshName;
        float effectType;
        xresource::full_guid textureGUID;
        xresource::full_guid normalMapGUID;

        bool operator==(const BatchKey& other) const {
            return meshName == other.meshName &&
                effectType == other.effectType &&
                textureGUID == other.textureGUID &&
                normalMapGUID == other.normalMapGUID;
        }

        bool operator<(const BatchKey& other) const {
            if (meshName != other.meshName) return meshName < other.meshName;
            if (effectType != other.effectType) return effectType < other.effectType;
            if (textureGUID.m_Instance.m_Value != other.textureGUID.m_Instance.m_Value)
                return textureGUID.m_Instance.m_Value < other.textureGUID.m_Instance.m_Value;
            if (textureGUID.m_Type.m_Value != other.textureGUID.m_Type.m_Value)
                return textureGUID.m_Type.m_Value < other.textureGUID.m_Type.m_Value;
            if (normalMapGUID.m_Instance.m_Value != other.normalMapGUID.m_Instance.m_Value)
                return normalMapGUID.m_Instance.m_Value < other.normalMapGUID.m_Instance.m_Value;
            return normalMapGUID.m_Type.m_Value < other.normalMapGUID.m_Type.m_Value;
        }
    };

    // ============================================================================
    // INSTANCE DATA - NO materialID needed, we store full material
    // ============================================================================

    struct InstanceData {
        glm::mat4 modelMatrix;
        glm::vec4 tintColor;

        // Material properties per-instance
        glm::vec3 Ka;
        float roughness;

        glm::vec3 Kd;
        float metallic;

        glm::vec3 Ks;
        float shininess;

        uint32_t effect;
        uint32_t padding[3];
    };

    // ============================================================================
    // INSTANCE BATCH
    // ============================================================================

    struct InstanceBatch {
        BatchKey key;

        std::string meshName;
        size_t meshIndexCount;
        GLuint meshVAO;

        GLuint instanceSSBO;
        std::vector<InstanceData> instanceData;
        size_t currentInstanceCount;
        size_t maxInstanceCapacity;

        bool isDeferredRenderable;
        bool needsBufferUpdate;

        // Material shared by all instances in this batch
        MaterialComponent batchMaterial;

        InstanceBatch()
            : instanceSSBO(0), currentInstanceCount(0), maxInstanceCapacity(0),
            isDeferredRenderable(true), needsBufferUpdate(false),
            meshVAO(0), meshIndexCount(0) {
        }

        ~InstanceBatch() {
            if (instanceSSBO != 0) {
                glDeleteBuffers(1, &instanceSSBO);
                instanceSSBO = 0;
            }
        }

        InstanceBatch(const InstanceBatch&) = delete;
        InstanceBatch& operator=(const InstanceBatch&) = delete;

        InstanceBatch(InstanceBatch&& other) noexcept
            : key(other.key), meshName(std::move(other.meshName)),
            meshIndexCount(other.meshIndexCount), meshVAO(other.meshVAO),
            instanceSSBO(other.instanceSSBO),
            instanceData(std::move(other.instanceData)),
            currentInstanceCount(other.currentInstanceCount),
            maxInstanceCapacity(other.maxInstanceCapacity),
            isDeferredRenderable(other.isDeferredRenderable),
            needsBufferUpdate(other.needsBufferUpdate),
            batchMaterial(other.batchMaterial) {
            other.instanceSSBO = 0;
        }
    };

    // ============================================================================
    // INSTANCING SYSTEM
    // ============================================================================

    class InstancingSystem {
    public:
        InstancingSystem();
        ~InstancingSystem();

        void Initialize(size_t maxInstancesPerBatch = 10000);
        void PrepareBatches();

        const std::map<BatchKey, InstanceBatch>& GetDeferredBatches() const {
            return m_deferredBatches;
        }

        const std::map<BatchKey, InstanceBatch>& GetForwardBatches() const {
            return m_forwardBatches;
        }

        void UpdateInstanceBuffer(InstanceBatch& batch);
        void ClearBatches();
        void PrintStatistics() const;

        size_t GetTotalDrawCalls() const {
            return m_deferredBatches.size() + m_forwardBatches.size();
        }

        size_t GetTotalInstanceCount() const;

    private:
        std::map<BatchKey, InstanceBatch> m_deferredBatches;
        std::map<BatchKey, InstanceBatch> m_forwardBatches;

        size_t m_maxInstancesPerBatch;

        BatchKey CreateBatchKey(const MaterialComponent& material,
            const MeshComponent& mesh,
            const TextureComponent* texComp,
            const NormalMapComponent* normalComp);

        void AddEntityToBatch(const BatchKey& key,
            const TransformComponent& transform,
            const MaterialComponent& material,
            bool isDeferredRenderable);

        void CreateOrResizeBuffer(InstanceBatch& batch);
    };
}
#endif // INSTANCINGSYSTEM_H