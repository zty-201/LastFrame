#include "pch.hpp"
#include "InstancingSystem.h"
#include "GraphicsSystem.h" 
#include "AssetManager.hpp"

namespace EngineFramework
{
    InstancingSystem::InstancingSystem() {}

    InstancingSystem::~InstancingSystem() {
        if (m_instanceSSBOs[0] != 0) {
            glDeleteBuffers(FRAMES_IN_FLIGHT, m_instanceSSBOs);
            glDeleteBuffers(FRAMES_IN_FLIGHT, m_indirectBuffers);
            glDeleteBuffers(FRAMES_IN_FLIGHT, m_cullingMatrixUBOs);
            glDeleteTextures(FRAMES_IN_FLIGHT, m_instanceTextures);
            glDeleteTextures(FRAMES_IN_FLIGHT, m_visibleIndicesTextures);
        }
    }

    void InstancingSystem::Initialize(size_t maxTotalInstances) {

        m_maxTotalInstances = maxTotalInstances;
		culling_shader_guid = AssetManagerSystem::GetFullGuid(AssetManagerSystem::SHADER_GUID, "Culling");

        // Generate buffers
        glGenBuffers(FRAMES_IN_FLIGHT, m_instanceSSBOs);
        glGenBuffers(FRAMES_IN_FLIGHT, m_indirectBuffers);
        glGenBuffers(FRAMES_IN_FLIGHT, m_cullingMatrixUBOs);

        glGenTextures(FRAMES_IN_FLIGHT, m_instanceTextures);
        glGenTextures(FRAMES_IN_FLIGHT, m_visibleIndicesTextures);

        // Pre-allocate memory for all frames in flight
        for (int i = 0; i < FRAMES_IN_FLIGHT; ++i) {
            // Instance Data
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_instanceSSBOs[i]);
            glBufferData(GL_SHADER_STORAGE_BUFFER, m_maxTotalInstances * sizeof(InstanceData), nullptr, GL_DYNAMIC_DRAW);

            // Indirect Commands
            glBindBuffer(GL_DRAW_INDIRECT_BUFFER, m_indirectBuffers[i]);
            glBufferData(GL_DRAW_INDIRECT_BUFFER, MAX_BATCHES * NUM_PASSES * sizeof(DrawElementsIndirectCommand), nullptr, GL_DYNAMIC_DRAW);

			// Culling Matrices UBO
            glBindBuffer(GL_UNIFORM_BUFFER, m_cullingMatrixUBOs[i]);
            // Pre-allocate the 8.4 KB block
            glBufferData(GL_UNIFORM_BUFFER, sizeof(CullingMatrixData), nullptr, GL_DYNAMIC_DRAW);

            int totalInstanceTexels = (int)m_maxTotalInstances * 6;
            m_instTexHeight = (totalInstanceTexels + m_instTexWidth - 1) / m_instTexWidth;
            int totalVisibleTexels = (int)(MAX_INSTANCES * NUM_PASSES);
            m_visTexHeight = (totalVisibleTexels + m_visTexWidth - 1) / m_visTexWidth;

            // Change instance texture back to float format — avoids Mali integer alignment bug
            glBindTexture(GL_TEXTURE_2D, m_instanceTextures[i]);
            glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, m_instTexWidth, m_instTexHeight);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

            // Visible indices stays as GL_R32UI — this one is fine since it's written by imageStore
            glBindTexture(GL_TEXTURE_2D, m_visibleIndicesTextures[i]);
            glTexStorage2D(GL_TEXTURE_2D, 1, GL_R32UI, m_visTexWidth, m_visTexHeight);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        }
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
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
            if (!meshData || meshData->subMeshes.empty()) continue;

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
            GLuint firstIndex = meshData->subMeshes[key.subMeshIndex].firstIndex; // Pulls Mega-IBO offset
            GLuint baseVert = meshData->subMeshes[key.subMeshIndex].baseVertex; // Pulls Mega-VBO offset

            // Setup base Indirect Commands
            for (int pass = 0; pass < NUM_PASSES; pass++) {
                int cmdIndex = (pass * MAX_BATCHES) + batchIndex;
                m_indirectCommands[cmdIndex].count = indexCount;
                m_indirectCommands[cmdIndex].instanceCount = 0;
                m_indirectCommands[cmdIndex].firstIndex = firstIndex;
                m_indirectCommands[cmdIndex].baseVertex = baseVert;   // Injected into hardware struct!
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

    }

    void InstancingSystem::UpdateAndDispatchCompute(
        const glm::mat4& mainViewProj,
        const std::array<glm::mat4, 3>& cascadeProj,
        const std::vector<glm::mat4>& spotProj,
        const std::vector<glm::mat4>& pointProj
    ) {

        // --- 1. HANDLE DIRTY BATCHES (From Level Editor UI) ---
        if (m_needsBatchRebuild) {
            BuildSceneBatches();
            m_needsBatchRebuild = false;
            m_pendingCreations.clear(); // Building batches catches everything, clear the queue
        }

        // --- 2. HANDLE DEFERRED CREATIONS (From Duplicate/Spawn) --- 
		// not in use currently as it requires a major refactor of how each systems
        // uses the transform and mesh component. 
        // Issue 1: when objects are destructed, we need to log the destruction of 
        // MeshInstance first as the destruction order arent guaranteed. 
        // There isnt frequent creation/destruction, 
        // but this is the solution for UI editor bug and future dynamic scenes

        if (!m_pendingCreations.empty()) {
            for (entt::entity newEntity : m_pendingCreations) {
                ProcessNewInstance(newEntity);
            }
            m_pendingCreations.clear();
        }

        if (m_globalInstanceData.empty()) return;

		// --- 3. UPDATE INSTANCE DATA & INDIRECT COMMANDS ---
		// triple buffering logic: we have 3 buffers for the instance data, 
        // indirect commands, and visible indices. Each frame, we write to 
        // the next buffer in a circular manner, ensuring the GPU is done 
        // with that buffer before we write to it again. 
        // This allows us to avoid stalls and ensures smooth rendering 
        // even when the CPU updates every frame.
        m_currentFrame++;
        int ringIdx = m_currentFrame % FRAMES_IN_FLIGHT;

        // 2. Update CPU cache (EnTT Loop)
        auto transform_view = registry.view<TransformComponent>();
        for (auto&& [entity, transform] : transform_view.each()) {
            if (transform.position != transform.prevPosition ||
                transform.rotation != transform.prevRotation ||
                transform.scale != transform.prevScale) {

                CalculateModelMatrix(transform, transform.modelMatrix);

                auto* meta = registry.try_get<InstanceMetadata>(entity);
                if (!meta) continue;

                for (uint32_t idx : meta->ssboIndices) {
                    m_globalInstanceData[idx].modelMatrix = transform.modelMatrix;
                }
            }
        }

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

        // 1. Set the standard integers
        cullingShader->setUniform("u_TotalInstances", (uint32_t)m_globalInstanceData.size());
        cullingShader->setUniform("u_NumSpotLights", (int)spotProj.size());
        cullingShader->setUniform("u_NumPointLightFaces", (int)pointProj.size());

        // 2. Pack all 132 matrices into our strict struct
        CullingMatrixData matrixData = {}; // Initializes with zeros
        matrixData.mainViewProj = mainViewProj;
        for (int i = 0; i < 3; i++) matrixData.cascadeViewProj[i] = cascadeProj[i];
        for (size_t i = 0; i < spotProj.size(); i++) matrixData.spotLightViewProj[i] = spotProj[i];
        for (size_t i = 0; i < pointProj.size(); i++) matrixData.pointLightViewProj[i] = pointProj[i];

        glBindBuffer(GL_UNIFORM_BUFFER, m_cullingMatrixUBOs[ringIdx]);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(CullingMatrixData), &matrixData);
        glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_cullingMatrixUBOs[ringIdx]);

        // Upload indirect commands
        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, m_indirectBuffers[ringIdx]);
        glBufferSubData(GL_DRAW_INDIRECT_BUFFER, 0,
            m_indirectCommands.size() * sizeof(DrawElementsIndirectCommand),
            m_indirectCommands.data());

        // Upload instance data to input SSBO (compute reads this)
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_instanceSSBOs[ringIdx]);
        glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0,
            m_globalInstanceData.size() * sizeof(InstanceData),
            m_globalInstanceData.data());

        // Upload instance data ALSO directly to texture (vertex shader reads this)
        int actualTexels = (int)m_globalInstanceData.size() * 6;
        int actualRows = (actualTexels + m_instTexWidth - 1) / m_instTexWidth;
        if (actualRows > 0) {
            glBindTexture(GL_TEXTURE_2D, m_instanceTextures[ringIdx]);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
                m_instTexWidth, actualRows,
                GL_RGBA, GL_FLOAT,
                m_globalInstanceData.data());
            glBindTexture(GL_TEXTURE_2D, 0);
        }

        // Bind compute inputs
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_instanceSSBOs[ringIdx]);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_indirectBuffers[ringIdx]);

        // Visible indices: written by compute directly into texture via imageStore
        glBindImageTexture(1, m_visibleIndicesTextures[ringIdx],
            0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32UI);

        GLuint numGroups = (GLuint)(m_globalInstanceData.size() + 63) / 64;
        glDispatchCompute(numGroups, 1, 1);

        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT |
            GL_TEXTURE_FETCH_BARRIER_BIT |
            GL_COMMAND_BARRIER_BIT);

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
        if (!meshData || meshData->subMeshes.empty()) return;

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

        }
    }
}