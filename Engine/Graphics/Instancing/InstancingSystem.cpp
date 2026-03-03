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

    void InstancingSystem::BuildSceneBatches()
    {
        m_globalInstanceData.clear();
        m_activeBatches.clear();
        m_animatedEntities.clear();
        registry.clear<InstanceMetadata>();

        // Pre-allocate indirect commands with zeros
        m_indirectCommands.assign(MAX_BATCHES * NUM_PASSES, DrawElementsIndirectCommand{ 0, 0, 0, 0, 0 });

        // Temporary struct to pair the entity with its data during sorting
        struct SubmeshInstance {
            entt::entity entity;
            InstanceData data;
        };

        std::map<BatchKey, std::vector<SubmeshInstance>> batchSortingMap;
        std::map<BatchKey, MaterialComponent> batchMaterials;

        auto transform_view = registry.view<TransformComponent>();
        for (auto&& [entity, transform] : transform_view.each()) {
            CalculateModelMatrix(transform, transform.modelMatrix);
        }

        auto view = registry.view<MeshComponent, TransformComponent>();

        for (auto [entity, meshComp, transformComp] : view.each()) {
            if (meshComp.meshName == "SkyBox" || !meshComp.visibility) continue;

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

                // Local bounding sphere (The compute shader converts this to world space!)
                glm::vec3 localMin = glm::vec3(meshComp.localMin[0], meshComp.localMin[1], meshComp.localMin[2]);
                glm::vec3 localMax = glm::vec3(meshComp.localMax[0], meshComp.localMax[1], meshComp.localMax[2]);
                glm::vec3 localCenter = (localMin + localMax) * 0.5f;
                float localRadius = glm::length(localMax - localMin) * 0.5f;

                InstanceData data;
                data.batchID = 0; // Filled below
                data.modelMatrix = transformComp.modelMatrix;
                data.boundingSphere = glm::vec4(localCenter, localRadius);

                batchSortingMap[key].push_back({ entity, data });
            }
        }

        // --- FLATTEN DATA AND BUILD COMMANDS ---
        size_t currentOffset = 0;
        int batchIndex = 0;

        for (auto& [key, instances] : batchSortingMap) {
            if (instances.empty()) continue;

            RenderBatch batch;
            batch.key = key;
            batch.material = batchMaterials[key];
            batch.instanceOffset = currentOffset;
            batch.instanceCount = instances.size();
            m_activeBatches.push_back(batch);

            auto meshData = AssetManagerSystem::GetResource<Mesh::MeshData>(
                AssetManagerSystem::GetFullGuid(AssetManagerSystem::MESH_GUID, key.meshName));

            GLuint indexCount = meshData->subMeshes[key.subMeshIndex].indexCount;
            GLuint firstIndex = meshData->subMeshes[key.subMeshIndex].baseIndex;

            // Setup base Indirect Commands
            for (int pass = 0; pass < NUM_PASSES; pass++) {
                int cmdIndex = (pass * MAX_BATCHES) + batchIndex;
                m_indirectCommands[cmdIndex].count = indexCount;
                m_indirectCommands[cmdIndex].instanceCount = 0;
                m_indirectCommands[cmdIndex].firstIndex = firstIndex;
                m_indirectCommands[cmdIndex].baseVertex = 0;
                m_indirectCommands[cmdIndex].baseInstance = 0;
            }
            size_t batchBaseOffset = currentOffset;
            for (auto& inst : instances) {
                inst.data.batchID = batchIndex;
                inst.data.instanceOffset = static_cast<uint32_t>(batchBaseOffset);

                m_globalInstanceData.push_back(inst.data);

                // Assign the GPU memory slot back to the ECS Entity
                auto& meta = registry.get_or_emplace<InstanceMetadata>(inst.entity);
                meta.ssboIndices.push_back(static_cast<uint32_t>(currentOffset));

                currentOffset++;
            }
            batchIndex++;
        }

        // --- INITIAL UPLOAD ---
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_instanceSSBO);
        glBufferData(GL_SHADER_STORAGE_BUFFER, m_maxTotalInstances * sizeof(InstanceData), nullptr, GL_DYNAMIC_DRAW);
        glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, m_globalInstanceData.size() * sizeof(InstanceData), m_globalInstanceData.data());
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }

    void InstancingSystem::UpdateAndDispatchCompute(const glm::mat4& mainViewProj, const std::array<glm::mat4, 3>& cascadeProj, const std::vector<glm::mat4>& spotProj) {
        if (m_globalInstanceData.empty()) return;

        // --- 1. PROCESS ONLY DIRTY ENTITIES ---
        /*auto transform_view = registry.view<TransformComponent>();
        for (auto&& [entity, transform] : transform_view.each()) {
            CalculateModelMatrix(transform, transform.modelMatrix);
        }*/
        auto transform_view = registry.view<TransformComponent>();
        bool buffer_bound = false; // Track if we need to bind the buffer

        for (auto&& [entity, transform] : transform_view.each()) {

            // Fast comparison of 9 floats
            if (transform.position != transform.prevPosition ||
                transform.rotation != transform.prevRotation ||
                transform.scale != transform.prevScale) {
                LOGI("Entity %zu has changed transform.", static_cast<size_t>(entity));
                // 2. DO THE HEAVY MATH ONLY WHEN NEEDED
                CalculateModelMatrix(transform, transform.modelMatrix);

				//LOGI("Entity %zu marked dirty. Updating GPU instance data.", static_cast<size_t>(entity));
                auto* meta = registry.try_get<InstanceMetadata>(entity);

                if (!meta) {
                    LOGW("Entity %zu has no InstanceMetadata! Skipping GPU update.", static_cast<size_t>(entity));
                    continue;
                }

                if (!buffer_bound) {
                    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_instanceSSBO);
                    buffer_bound = true;
                }
                for (uint32_t idx : meta->ssboIndices) {
                    // 1. Update the CPU-side cache
                    m_globalInstanceData[idx].modelMatrix = transform.modelMatrix;

                    // 2. Upload ONLY this specific struct to the GPU
                    // Offset = index * size of one struct
                    // Size = size of one struct
                    glBufferSubData(GL_SHADER_STORAGE_BUFFER,
                        idx * sizeof(InstanceData),
                        sizeof(InstanceData),
                        &m_globalInstanceData[idx]);
                }
                
            }
        }
        if (buffer_bound) {
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
        }

        // --- 3. RESET INDIRECT COMMANDS (Instance Counts -> 0) ---
        // The GPU overwrites these counts every frame, so we must reset them using our pristine CPU copy
        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, m_indirectBuffer);
        glBufferData(GL_DRAW_INDIRECT_BUFFER, MAX_BATCHES * NUM_PASSES * sizeof(DrawElementsIndirectCommand), nullptr, GL_DYNAMIC_DRAW);
        glBufferSubData(GL_DRAW_INDIRECT_BUFFER, 0, m_indirectCommands.size() * sizeof(DrawElementsIndirectCommand), m_indirectCommands.data());

        // --- 4. DISPATCH COMPUTE SHADER ---
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

        // For directional light cascade culling
        cullingShader->setUniform("u_MainViewProj", mainViewProj);
        for (int i = 0; i < 3; i++) {
            cullingShader->setUniform("u_CascadeViewProj[" + std::to_string(i) + "]", cascadeProj[i]);
        }

		// For spotlight culling
        cullingShader->setUniform("u_NumSpotLights", (int)spotProj.size());
        for (size_t i = 0; i < spotProj.size(); ++i) {
            cullingShader->setUniform("u_SpotLightViewProj[" + std::to_string(i) + "]", spotProj[i]);
        }

        cullingShader->setUniform("u_TotalInstances", (uint32_t)m_globalInstanceData.size());

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_instanceSSBO);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_indirectBuffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, m_visibleIndicesBuffer);

        GLuint numGroups = (GLuint)(m_globalInstanceData.size() + 63) / 64;
        glDispatchCompute(numGroups, 1, 1);
        glMemoryBarrier(GL_COMMAND_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);

        cullingShader->disuse();
        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }

    void InstancingSystem::CalculateModelMatrix(TransformComponent& transform, glm::mat4& modelMatrix)
    {
        // Store previous frame's transform data 
        transform.prevPosition = transform.position;
        transform.prevRotation = transform.rotation;
        transform.prevScale = transform.scale;

        // Translation
        glm::mat4 T = glm::translate(glm::mat4(1.0f), transform.position);

        // Rotation (Euler - Quaternion - Matrix)
        glm::vec3 rotationRadians = glm::radians(transform.rotation);
        glm::quat Q = glm::quat(rotationRadians);
        glm::mat4 R = glm::mat4_cast(Q);

        // Scale
        glm::mat4 S = glm::scale(glm::mat4(1.0f), transform.scale);

        // Combine: TRS
        glm::mat4 model = T * R * S;
        modelMatrix = model;
        //transform.prevModelMatrix = modelMatrix; // Store the initial model matrix as the previous one
    }
}