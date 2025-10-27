#include "pch.hpp"
#include "InstancingSystem.h"
namespace EngineFramework
{
    InstancingSystem::InstancingSystem()
        : m_maxInstancesPerBatch(10000) {
    }

    InstancingSystem::~InstancingSystem() {
    }

    void InstancingSystem::Initialize(size_t maxInstancesPerBatch) {
        m_maxInstancesPerBatch = maxInstancesPerBatch;
        LOGI("InstancingSystem initialized with max %zu instances per batch", maxInstancesPerBatch);
    }

    void InstancingSystem::ClearBatches() {
        m_deferredBatches.clear();
        m_forwardBatches.clear();
    }

    BatchKey InstancingSystem::CreateBatchKey(const MaterialComponent& material,
        const MeshComponent& mesh,
        const TextureComponent* texComp,
        const NormalMapComponent* normalComp) {
        BatchKey key;
        key.meshName = mesh.meshName;
        key.effectType = material.effect;
        key.textureGUID = texComp ? texComp->texture_guid : xresource::full_guid{};
        key.normalMapGUID = normalComp ? normalComp->normalmap_guid : xresource::full_guid{};
        return key;
    }

    void InstancingSystem::AddEntityToBatch(const BatchKey& key,
        const TransformComponent& transform,
        const MaterialComponent& material,
        bool isDeferredRenderable) {
        auto& targetBatches = isDeferredRenderable ? m_deferredBatches : m_forwardBatches;

        auto it = targetBatches.find(key);
        InstanceBatch* batch = nullptr;

        if (it == targetBatches.end()) {
            InstanceBatch newBatch;
            newBatch.key = key;
            newBatch.meshName = key.meshName;
            newBatch.isDeferredRenderable = isDeferredRenderable;
            newBatch.batchMaterial = material;

            batch = &targetBatches.insert({ key, std::move(newBatch) }).first->second;
            CreateOrResizeBuffer(*batch);
        }
        else {
            batch = &it->second;
        }

        if (batch->currentInstanceCount >= batch->maxInstanceCapacity) {
            LOGE("Batch capacity exceeded for %s, ignoring entity", key.meshName.c_str());
            return;
        }

        InstanceData instanceData;
        instanceData.modelMatrix = transform.modelMatrix;
        instanceData.tintColor = glm::vec4(material.color, 1.0f);

        // Store material properties per-instance
        instanceData.Ka = material.Ka;
        instanceData.Kd = material.Kd;
        instanceData.Ks = material.Ks;
        instanceData.roughness = material.rough;
        instanceData.metallic = material.metal;
        instanceData.shininess = material.shininess;
        instanceData.effect = static_cast<uint32_t>(material.effect);

        batch->instanceData.push_back(instanceData);
        batch->currentInstanceCount++;
        batch->needsBufferUpdate = true;
    }

    void InstancingSystem::CreateOrResizeBuffer(InstanceBatch& batch) {
        size_t requiredSize = m_maxInstancesPerBatch * sizeof(InstanceData);

        if (batch.instanceSSBO == 0) {
            glGenBuffers(1, &batch.instanceSSBO);
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, batch.instanceSSBO);
            glBufferData(GL_SHADER_STORAGE_BUFFER, requiredSize, nullptr, GL_DYNAMIC_DRAW);
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

            batch.maxInstanceCapacity = m_maxInstancesPerBatch;
            LOGD("Created SSBO for batch %s (capacity: %zu)", batch.meshName.c_str(), m_maxInstancesPerBatch);
        }
    }

    void InstancingSystem::UpdateInstanceBuffer(InstanceBatch& batch) {
        if (!batch.needsBufferUpdate || batch.currentInstanceCount == 0) {
            return;
        }

        size_t dataSize = batch.currentInstanceCount * sizeof(InstanceData);

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, batch.instanceSSBO);
        glBufferData(GL_SHADER_STORAGE_BUFFER, batch.maxInstanceCapacity * sizeof(InstanceData),
            nullptr, GL_DYNAMIC_DRAW);
        glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, dataSize, batch.instanceData.data());
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

        batch.needsBufferUpdate = false;
        LOGD("Updated SSBO for batch %s: %zu instances", batch.meshName.c_str(), batch.currentInstanceCount);
    }

    void InstancingSystem::PrepareBatches() {
        ClearBatches();

        auto scene_view = registry.view<TransformComponent, MaterialComponent, MeshComponent>();

        for (auto [entity, transform, material, mesh] : scene_view.each()) {
            bool isDeferredRenderable = (material.effect == 0.0f);

            auto* texComp = registry.try_get<TextureComponent>(entity);
            auto* normalComp = registry.try_get<NormalMapComponent>(entity);

            BatchKey key = CreateBatchKey(material, mesh, texComp, normalComp);
            AddEntityToBatch(key, transform, material, isDeferredRenderable);
        }

        // Update all buffers
        for (auto& [key, batch] : m_deferredBatches) {
            UpdateInstanceBuffer(batch);
        }
        for (auto& [key, batch] : m_forwardBatches) {
            UpdateInstanceBuffer(batch);
        }
    }

    size_t InstancingSystem::GetTotalInstanceCount() const {
        size_t total = 0;
        for (const auto& [key, batch] : m_deferredBatches) {
            total += batch.currentInstanceCount;
        }
        for (const auto& [key, batch] : m_forwardBatches) {
            total += batch.currentInstanceCount;
        }
        return total;
    }

    void InstancingSystem::PrintStatistics() const {
        LOGI("=== INSTANCING STATISTICS ===");
        LOGI("Deferred batches: %zu", m_deferredBatches.size());
        for (const auto& [key, batch] : m_deferredBatches) {
            LOGI("  Mesh: %s | Instances: %zu", key.meshName.c_str(), batch.currentInstanceCount);
        }

        LOGI("Forward batches: %zu", m_forwardBatches.size());
        for (const auto& [key, batch] : m_forwardBatches) {
            LOGI("  Mesh: %s | Instances: %zu (Effect: %.1f)", key.meshName.c_str(),
                batch.currentInstanceCount, key.effectType);
        }

        LOGI("Total draw calls: %zu", GetTotalDrawCalls());
        LOGI("Total instances: %zu", GetTotalInstanceCount());
        LOGI("===============================");
    }
}