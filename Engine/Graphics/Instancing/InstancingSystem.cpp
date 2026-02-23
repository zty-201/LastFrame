#include "pch.hpp"
#include "InstancingSystem.h"
#include "GraphicsSystem.h" 
#include "AssetManager.hpp"

namespace EngineFramework
{
    InstancingSystem::InstancingSystem() {}

    InstancingSystem::~InstancingSystem() {
        if (m_instanceSSBO != 0) {
            glDeleteBuffers(1, &m_instanceSSBO);
        }
    }

    void InstancingSystem::Initialize(size_t maxTotalInstances) {
        m_maxTotalInstances = maxTotalInstances;

        // Pre-allocate the massive SSBO once
        glGenBuffers(1, &m_instanceSSBO);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_instanceSSBO);
        glBufferData(GL_SHADER_STORAGE_BUFFER, m_maxTotalInstances * sizeof(InstanceData), nullptr, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

        // Reserve memory to prevent CPU reallocations during gathering
        m_globalInstanceData.reserve(m_maxTotalInstances);
        LOGI("InstancingSystem initialized with capacity for %zu instances.", m_maxTotalInstances);
    }

    void InstancingSystem::GatherPass(const glm::mat4& viewProj) {
        m_globalInstanceData.clear();
        m_activeBatches.clear();
        m_animatedEntities.clear();

        // 1. Temporary map to sort instances
        std::map<BatchKey, std::vector<InstanceData>> batchSortingMap;
        std::map<BatchKey, MaterialComponent> batchMaterials;

        auto view = registry.view<MeshComponent, TransformComponent>();

        for (auto [entity, meshComp, transformComp] : view.each()) {
            if (meshComp.meshName == "SkyBox" || !meshComp.visibility) continue;

            // Frustum Culling via GraphicsSystem
            if (!GraphicsSystem::GetInstance().IsBoxInFrustum(meshComp.localMin, meshComp.localMax, transformComp.modelMatrix, viewProj)) {
                continue;
            }

            // Separate Animated Entities (They get drawn normally)
            if (registry.any_of<AnimationComponent>(entity)) {
                m_animatedEntities.push_back(entity);
                continue;
            }

            // Get Mesh Data to find submeshes and default materials
            xresource::full_guid mesh_guid = AssetManagerSystem::GetFullGuid(AssetManagerSystem::MESH_GUID, meshComp.meshName);
            auto meshData = AssetManagerSystem::GetResource<Mesh::MeshData>(mesh_guid);

            if (!meshData || meshData->vao == 0) continue;

            // Check if entity has a material override
            const MaterialComponent* overrideMat = registry.try_get<MaterialComponent>(entity);

            for (size_t i = 0; i < meshData->subMeshes.size(); i++) {
                const auto& subMesh = meshData->subMeshes[i];
                if (subMesh.materialIndex < 0 || subMesh.materialIndex >= meshData->materials.size()) continue;

                // Determine final material
                const MaterialComponent& mat = overrideMat ? *overrideMat : meshData->materials[subMesh.materialIndex];

                // Construct fingerprint
                BatchKey key;
                key.meshName = meshComp.meshName;
                key.subMeshIndex = static_cast<int>(i);
                key.albedoMap = mat.albedoMap;
                key.normalMap = mat.normalMap;
                key.roughnessMap = mat.roughnessMap;
                key.metallicMap = mat.metallicMap;
                key.albedoFactor = mat.albedoFactor;
                key.roughnessFactor = mat.roughnessFactor;
                key.metallicFactor = mat.metallicFactor;
                key.uvTiling = mat.uvTiling;
                key.uvOffset = mat.uvOffset;

                // Save material state for this batch if it's new
                if (batchMaterials.find(key) == batchMaterials.end()) {
                    batchMaterials[key] = mat;
                }

                // Add to temporary sorting map
                batchSortingMap[key].push_back({ transformComp.modelMatrix });
            }
        }

        // 2. Flatten the sorted map into our continuous vectors
        size_t currentOffset = 0;

        for (auto& [key, instances] : batchSortingMap) {
            if (instances.empty()) continue;

            // Prevent buffer overflow
            if (currentOffset + instances.size() > m_maxTotalInstances) {
                LOGE("Instancing buffer overflow! Exceeded %zu instances.", m_maxTotalInstances);
                break;
            }

            // Create the final RenderBatch
            RenderBatch batch;
            batch.key = key;
            batch.material = batchMaterials[key];
            batch.instanceOffset = currentOffset;
            batch.instanceCount = instances.size();
            m_activeBatches.push_back(batch);

            // Copy data into global flat array
            m_globalInstanceData.insert(m_globalInstanceData.end(), instances.begin(), instances.end());
            currentOffset += instances.size();
        }

        // 3. Upload exactly ONE time to the GPU
        if (!m_globalInstanceData.empty()) {
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_instanceSSBO);
            // Orphan the old buffer to prevent CPU/GPU sync stalls
            glBufferData(GL_SHADER_STORAGE_BUFFER, m_maxTotalInstances * sizeof(InstanceData), nullptr, GL_DYNAMIC_DRAW);
            glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, m_globalInstanceData.size() * sizeof(InstanceData), m_globalInstanceData.data());
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
        }
    }
}