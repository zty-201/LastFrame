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
		culling_shader_guid = AssetManagerSystem::GetFullGuid(AssetManagerSystem::SHADER_GUID, "Culling");
        // Pre-allocate the massive SSBO once
        glGenBuffers(1, &m_instanceSSBO);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_instanceSSBO);
        glBufferData(GL_SHADER_STORAGE_BUFFER, m_maxTotalInstances * sizeof(InstanceData), nullptr, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

        // Allocate Indirect Command Buffer (MAX_BATCHES * 4 passes)
        glGenBuffers(1, &m_indirectBuffer);
        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, m_indirectBuffer);
        glBufferData(GL_DRAW_INDIRECT_BUFFER, MAX_BATCHES * NUM_PASSES * sizeof(DrawElementsIndirectCommand), nullptr, GL_DYNAMIC_DRAW);

        // Allocate Visible Indices Buffer (MAX_INSTANCES * 4 passes)
        glGenBuffers(1, &m_visibleIndicesBuffer);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_visibleIndicesBuffer);
        glBufferData(GL_SHADER_STORAGE_BUFFER, MAX_INSTANCES * NUM_PASSES * sizeof(GLuint), nullptr, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

        // Reserve memory to prevent CPU reallocations during gathering
        m_globalInstanceData.reserve(m_maxTotalInstances);
        m_indirectCommands.reserve(1000);
        LOGI("InstancingSystem initialized with capacity for %zu instances.", m_maxTotalInstances);
    }

    void InstancingSystem::GatherPass(const glm::mat4& mainViewProj, const std::array<glm::mat4, 3>& cascadeProj)
    {
        m_globalInstanceData.clear();
        m_activeBatches.clear();
        m_animatedEntities.clear();
        m_indirectCommands.clear(); // Ensure this is a std::vector<DrawElementsIndirectCommand>

        std::map<BatchKey, std::vector<InstanceData>> batchSortingMap;
        std::map<BatchKey, MaterialComponent> batchMaterials;

        auto view = registry.view<MeshComponent, TransformComponent>();

        for (auto [entity, meshComp, transformComp] : view.each()) {
            if (meshComp.meshName == "SkyBox" || !meshComp.visibility) continue;

            // NOTICE: CPU Frustum culling is completely removed here!

            // Separate Animated Entities
            if (registry.any_of<AnimationComponent>(entity)) {
                m_animatedEntities.push_back(entity);
                continue;
            }

            // Get Mesh Data
            xresource::full_guid mesh_guid = AssetManagerSystem::GetFullGuid(AssetManagerSystem::MESH_GUID, meshComp.meshName);
            auto meshData = AssetManagerSystem::GetResource<Mesh::MeshData>(mesh_guid);
            if (!meshData || meshData->vao == 0) continue;

            const MaterialComponent* overrideMat = registry.try_get<MaterialComponent>(entity);

            for (size_t i = 0; i < meshData->subMeshes.size(); i++) {
                const auto& subMesh = meshData->subMeshes[i];
                if (subMesh.materialIndex < 0 || subMesh.materialIndex >= meshData->materials.size()) continue;

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

                if (batchMaterials.find(key) == batchMaterials.end()) {
                    batchMaterials[key] = mat;
                }

                // --- CALCULATE BOUNDING SPHERE ---
                glm::vec3 localMin = glm::vec3(meshComp.localMin[0], meshComp.localMin[1], meshComp.localMin[2]);
                glm::vec3 localMax = glm::vec3(meshComp.localMax[0], meshComp.localMax[1], meshComp.localMax[2]);
                glm::vec3 localCenter = (localMin + localMax) * 0.5f;
                float localRadius = glm::length(localMax - localMin) * 0.5f;

                InstanceData data;
                data.batchID = 0;
                data.modelMatrix = transformComp.modelMatrix;
                data.boundingSphere = glm::vec4(localCenter, localRadius);

                batchSortingMap[key].push_back(data);
            }
        }

        // --- FLATTEN DATA AND BUILD COMMANDS ---
        size_t currentOffset = 0;
        int batchIndex = 0;
        //size_t numBatches = batchSortingMap.size();

        // Allocate exact space for 4 passes (Main + 3 Cascades)
        m_indirectCommands.assign(MAX_BATCHES * NUM_PASSES, DrawElementsIndirectCommand{ 0, 0, 0, 0, 0 });

        for (auto& [key, instances] : batchSortingMap) {
            if (instances.empty()) continue;
            if (currentOffset + instances.size() > m_maxTotalInstances) break;

            RenderBatch batch;
            batch.key = key;
            batch.material = batchMaterials[key];
            batch.instanceOffset = currentOffset;
            batch.instanceCount = instances.size();
            m_activeBatches.push_back(batch);

            auto meshData = AssetManagerSystem::GetResource<Mesh::MeshData>(
                AssetManagerSystem::GetFullGuid(AssetManagerSystem::MESH_GUID, key.meshName));

            if (meshData && key.subMeshIndex < meshData->subMeshes.size()) {
                GLuint indexCount = meshData->subMeshes[key.subMeshIndex].indexCount;
                GLuint firstIndex = meshData->subMeshes[key.subMeshIndex].baseIndex;

                // Base Instance is always 0 because the Compute Shader handles indexing now
                for (int pass = 0; pass < 4; pass++) {
                    int cmdIndex = (pass * MAX_BATCHES) + batchIndex;
                    m_indirectCommands[cmdIndex].count = indexCount;
                    m_indirectCommands[cmdIndex].instanceCount = 0; // GPU WILL FILL THIS!
                    m_indirectCommands[cmdIndex].firstIndex = firstIndex;
                    m_indirectCommands[cmdIndex].baseVertex = 0;
                    m_indirectCommands[cmdIndex].baseInstance = 0;
                }
            }

            for (auto& instance : instances) {
                instance.batchID = batchIndex;
                instance.instanceOffset = static_cast<uint32_t>(currentOffset);
            }

            m_globalInstanceData.insert(m_globalInstanceData.end(), instances.begin(), instances.end());
            currentOffset += instances.size();
            batchIndex++;
        }

        if (m_globalInstanceData.empty()) return;

        // --- UPLOAD DATA TO GPU ---
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_instanceSSBO);
        // Orphan the old buffer
        glBufferData(GL_SHADER_STORAGE_BUFFER, m_maxTotalInstances * sizeof(InstanceData), nullptr, GL_DYNAMIC_DRAW);
        // Upload to the new buffer
        glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, m_globalInstanceData.size() * sizeof(InstanceData), m_globalInstanceData.data());

        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, m_indirectBuffer);
        // Orphan the old buffer
        glBufferData(GL_DRAW_INDIRECT_BUFFER, MAX_BATCHES* NUM_PASSES * sizeof(DrawElementsIndirectCommand), nullptr, GL_DYNAMIC_DRAW);
        // Upload to the new buffer
        glBufferSubData(GL_DRAW_INDIRECT_BUFFER, 0, m_indirectCommands.size() * sizeof(DrawElementsIndirectCommand), m_indirectCommands.data());

        // --- DISPATCH COMPUTE SHADER ---
        // Make sure you load your Culling Compute Shader in GraphicsSystem::SetupGraphics
        std::shared_ptr<ShaderProgram> cullingShader{};
        if (culling_shader_guid != xresource::full_guid{})
        {
            cullingShader = EngineFramework::AssetManagerSystem::GetResource<ShaderProgram>(culling_shader_guid);
        }
        else
        {
            using E_AM = EngineFramework::AssetManagerSystem;
            culling_shader_guid = E_AM::GetFullGuid(E_AM::SHADER_GUID, "Culling");
            cullingShader = E_AM::GetResource<ShaderProgram>(culling_shader_guid);
        }
        cullingShader->use();

        cullingShader->setUniform("u_MainViewProj", mainViewProj);
        for (int i = 0; i < 3; i++) {
            cullingShader->setUniform("u_CascadeViewProj[" + std::to_string(i) + "]", cascadeProj[i]);
        }
        cullingShader->setUniform("u_TotalInstances", (uint32_t)m_globalInstanceData.size());
        //cullingShader->setUniform("u_TotalBatches", (uint32_t)numBatches);

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_instanceSSBO);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_indirectBuffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, m_visibleIndicesBuffer);

        // 1 thread per instance
        GLuint numGroups = (GLuint)(m_globalInstanceData.size() + 63) / 64;
        glDispatchCompute(numGroups, 1, 1);

        // Block CPU until compute is done writing
        glMemoryBarrier(GL_COMMAND_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);

        cullingShader->disuse();
        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }
}