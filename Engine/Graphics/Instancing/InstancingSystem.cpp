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

        // --- 1. HANDLE DIRTY BATCHES (From Level Editor UI) ---
        if (m_needsBatchRebuild) {
            BuildSceneBatches();
            m_needsBatchRebuild = false;
            m_pendingCreations.clear(); // Building batches catches everything, clear the queue!
        }

        // --- 2. HANDLE DEFERRED CREATIONS (From Duplicate/Spawn) ---
        if (!m_pendingCreations.empty()) {
            for (entt::entity newEntity : m_pendingCreations) {
                ProcessNewInstance(newEntity);
            }
            m_pendingCreations.clear();
        }

        if (m_globalInstanceData.empty()) return;

        // --- 1. PROCESS ONLY DIRTY ENTITIES ---
        auto transform_view = registry.view<TransformComponent>();
        uint32_t minDirtyIndex = std::numeric_limits<uint32_t>::max();
        uint32_t maxDirtyIndex = 0;
        bool hasDirtyInstances = false;

        for (auto&& [entity, transform] : transform_view.each()) {

            // Fast comparison of 9 floats
            if (transform.position != transform.prevPosition ||
                transform.rotation != transform.prevRotation ||
                transform.scale != transform.prevScale) {
				// Calculate model matrix only when dirty
                CalculateModelMatrix(transform, transform.modelMatrix);

                auto* meta = registry.try_get<InstanceMetadata>(entity);

                if (!meta) continue;
                
                for (uint32_t idx : meta->ssboIndices) {
                    // 1. Update the CPU-side cache
                    m_globalInstanceData[idx].modelMatrix = transform.modelMatrix;

                    // 2. Expand our memory "bounding box"
                    minDirtyIndex = std::min(minDirtyIndex, idx);
                    maxDirtyIndex = std::max(maxDirtyIndex, idx);
                    hasDirtyInstances = true;
                }
            }
        }

        // --- 2. SINGLE BATCHED UPLOAD ---
        if (hasDirtyInstances) {
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_instanceSSBO);

            // Calculate the contiguous chunk size and starting offset
            uint32_t updateCount = (maxDirtyIndex - minDirtyIndex) + 1;
            GLintptr offset = minDirtyIndex * sizeof(InstanceData);
            GLsizeiptr size = updateCount * sizeof(InstanceData);

            // Upload the specific slice of the vector
            glBufferSubData(
                GL_SHADER_STORAGE_BUFFER,
                offset,
                size,
                &m_globalInstanceData[minDirtyIndex]
            );

            glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
            //glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_instanceSSBO);

            //// This single line does both:
            //// 1. Abandons the old memory (bypassing the GPU stall)
            //// 2. Uploads the entire CPU vector into the new memory block
            //glBufferData(
            //    GL_SHADER_STORAGE_BUFFER,
            //    m_globalInstanceData.size() * sizeof(InstanceData),
            //    m_globalInstanceData.data(),
            //    GL_DYNAMIC_DRAW
            //);

            //glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
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

    void InstancingSystem::SetDynamicUpdatesEnabled(bool enabled)
    {
        if (enabled == m_isListening) return;

        if (enabled) {
            // All reactions changed to bathc breaking change for now, implement optimization later
            // 1. Listen for Creations / Deletions
            registry.on_construct<MeshComponent>().connect<&InstancingSystem::OnBatchBreakingChange>(this);
            registry.on_destroy<MeshComponent>().connect<&InstancingSystem::OnBatchBreakingChange>(this);

            // 2. Listen for Batch-Breaking Modifications (The fix for your UI editor bug)
            registry.on_update<MaterialComponent>().connect<&InstancingSystem::OnBatchBreakingChange>(this);
            registry.on_update<MeshComponent>().connect<&InstancingSystem::OnBatchBreakingChange>(this);
            registry.on_update<AnimationComponent>().connect<&InstancingSystem::OnBatchBreakingChange>(this);

            m_isListening = true;
        }
        else {
            // Disconnect everything
            registry.on_construct<MeshComponent>().disconnect<&InstancingSystem::OnBatchBreakingChange>(this);
            registry.on_destroy<MeshComponent>().disconnect<&InstancingSystem::OnBatchBreakingChange>(this);

            registry.on_update<MaterialComponent>().disconnect<&InstancingSystem::OnBatchBreakingChange>(this);
            registry.on_update<MeshComponent>().disconnect<&InstancingSystem::OnBatchBreakingChange>(this);
            registry.on_update<AnimationComponent>().disconnect<&InstancingSystem::OnBatchBreakingChange>(this);

            // Clear pending states
            m_pendingCreations.clear();
            m_needsBatchRebuild = false;
            m_isListening = false;
        }
    }

    void InstancingSystem::OnBatchBreakingChange(entt::registry& reg, entt::entity entity)
    {
        UNREFERENCED_PARAMETER(reg);
        UNREFERENCED_PARAMETER(entity);
        m_needsBatchRebuild = true;
    }

    void InstancingSystem::OnMeshAdded(entt::registry& reg, entt::entity entity)
    {
        UNREFERENCED_PARAMETER(reg);
        // Defer the processing to avoid the ECS initialization crash!
        m_pendingCreations.push_back(entity);
		LOGI("MeshComponent added to Entity %u, deferring batch processing.", static_cast<uint32_t>(entity));
    }

    void InstancingSystem::OnMeshRemoved(entt::registry& reg, entt::entity entity)
    {
        auto* meta = reg.try_get<InstanceMetadata>(entity);
        if (!meta) return;

        for (uint32_t idx : meta->ssboIndices) {
            // 1. Mark as Tombstone so the Compute Shader skips it
            m_globalInstanceData[idx].batchID = INVALID_BATCH;

            // 2. Add to free list for future use
            m_freeSSBOIndices.push_back(idx);

            // 3. Upload the tombstone to the GPU immediately (Just 80 bytes!)
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_instanceSSBO);
            glBufferSubData(GL_SHADER_STORAGE_BUFFER, idx * sizeof(InstanceData), sizeof(InstanceData), &m_globalInstanceData[idx]);
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
        }
		LOGI("MeshComponent removed from Entity %u, marked %zu instances for deletion.", static_cast<uint32_t>(entity), meta->ssboIndices.size());
    }

    void InstancingSystem::ProcessNewInstance(entt::entity entity)
    {
        auto* meshComp = registry.try_get<MeshComponent>(entity);
        auto* transformComp = registry.try_get<TransformComponent>(entity);

        if (!meshComp || !transformComp || meshComp->meshName == "SkyBox" || !meshComp->visibility) return;

        // We don't batch animated entities in the SSBO the same way
        if (registry.any_of<AnimationComponent>(entity)) {
            m_animatedEntities.push_back(entity);
            return;
        }

        xresource::full_guid mesh_guid = AssetManagerSystem::GetFullGuid(AssetManagerSystem::MESH_GUID, meshComp->meshName);
        auto meshData = AssetManagerSystem::GetResource<Mesh::MeshData>(mesh_guid);
        if (!meshData || meshData->vao == 0) return;

        const MaterialComponent* overrideMat = registry.try_get<MaterialComponent>(entity);
        auto& meta = registry.get_or_emplace<InstanceMetadata>(entity);

        for (size_t i = 0; i < meshData->subMeshes.size(); i++) {
            const auto& subMesh = meshData->subMeshes[i];
            if (subMesh.materialIndex < 0 || subMesh.materialIndex >= meshData->materials.size()) continue;

            const MaterialComponent& mat = overrideMat ? *overrideMat : meshData->materials[subMesh.materialIndex];

            // 1. Construct the Fingerprint
            BatchKey key;
            key.meshName = meshComp->meshName;
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

            // 2. Find the existing batch
            int targetBatchID = -1;
            uint32_t targetInstanceOffset = 0;
            for (size_t b = 0; b < m_activeBatches.size(); b++) {
                if (!(m_activeBatches[b].key < key) && !(key < m_activeBatches[b].key)) { // Key equality check
                    targetBatchID = static_cast<int>(b);
                    targetInstanceOffset = static_cast<uint32_t>(m_activeBatches[b].instanceOffset);
                    m_activeBatches[b].instanceCount++; // Increment CPU count
                    break;
                }
            }

            // 3. Fallback: If this is a brand new mesh/material combo never seen before
            if (targetBatchID == -1) {
                BuildSceneBatches();
                return; // BuildSceneBatches handles everything, so we can just exit.
            }

            // 4. Create the new Data
            glm::vec3 localMin = glm::vec3(meshComp->localMin[0], meshComp->localMin[1], meshComp->localMin[2]);
            glm::vec3 localMax = glm::vec3(meshComp->localMax[0], meshComp->localMax[1], meshComp->localMax[2]);
            glm::vec3 localCenter = (localMin + localMax) * 0.5f;
            float localRadius = glm::length(localMax - localMin) * 0.5f;

            InstanceData data;
            data.batchID = targetBatchID;
            data.instanceOffset = targetInstanceOffset;
            CalculateModelMatrix(*transformComp, data.modelMatrix);
            data.boundingSphere = glm::vec4(localCenter, localRadius);

            // 5. Find a memory slot (Free List or Append)
            uint32_t targetIdx = 0;
            if (!m_freeSSBOIndices.empty()) {
                targetIdx = m_freeSSBOIndices.back();
                m_freeSSBOIndices.pop_back();
                m_globalInstanceData[targetIdx] = data;
            }
            else {
                targetIdx = static_cast<uint32_t>(m_globalInstanceData.size());
                m_globalInstanceData.push_back(data);
            }

            meta.ssboIndices.push_back(targetIdx);

            // 6. Upload ONLY the new data to the GPU (80 bytes!)
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_instanceSSBO);
            glBufferSubData(GL_SHADER_STORAGE_BUFFER, targetIdx * sizeof(InstanceData), sizeof(InstanceData), &m_globalInstanceData[targetIdx]);
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
        }
    }
}