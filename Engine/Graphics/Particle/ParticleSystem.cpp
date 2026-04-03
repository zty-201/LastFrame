#include "pch.hpp"
#include "ParticleSystem.h"
#include "AssetManager.hpp"
#include "GraphicsSystem.h"

namespace EngineFramework {

    ParticleSystem::ParticleSystem() {}

    ParticleSystem::~ParticleSystem() {
        if (m_particleSSBO != 0) glDeleteBuffers(1, &m_particleSSBO);
    }

    void ParticleSystem::Initialize() {
        glGenBuffers(1, &m_particleSSBO);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_particleSSBO);

        // Fill VRAM with dead particles (life = 0.0)
        std::vector<ParticleData> emptyData(m_maxParticles, { glm::vec4(0.0f), glm::vec4(0.0f), glm::vec4(0.0f) });
        glBufferData(GL_SHADER_STORAGE_BUFFER, m_maxParticles * sizeof(ParticleData), emptyData.data(), GL_DYNAMIC_DRAW);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

        using E_AM = EngineFramework::AssetManagerSystem;
        m_computeShaderGuid = E_AM::GetFullGuid(E_AM::SHADER_GUID, "ParticleSimulation");
        m_renderShaderGuid = E_AM::GetFullGuid(E_AM::SHADER_GUID, "ParticleRender");

        m_newParticlesThisFrame.reserve(2000);
    }

    void ParticleSystem::Update(float deltaTime) {
        m_newParticlesThisFrame.clear();

        auto view = registry.view<ParticleEmitterComponent, TransformComponent>();

        for (auto [entity, emitter, transform] : view.each()) {
            if (!emitter.isEmitting) continue;

            emitter.timeSinceLastSpawn += deltaTime;
            float timeBetweenSpawns = 1.0f / emitter.spawnRate;

            while (emitter.timeSinceLastSpawn >= timeBetweenSpawns) {
                emitter.timeSinceLastSpawn -= timeBetweenSpawns;

                ParticleData p;
                glm::vec3 worldPos = glm::vec3(transform.modelMatrix * glm::vec4(emitter.localOffset, 1.0f));

				// Ensure maxLifetime is not less than minLifetime to avoid invalid random range
                if (emitter.maxLifetime < emitter.minLifetime) {
                    emitter.maxLifetime = emitter.minLifetime;
				}

                p.position_life = glm::vec4(worldPos, RandomFloat(emitter.minLifetime, emitter.maxLifetime));

                // Shoot upward with a slight random spread
                glm::vec3 dir = glm::normalize(glm::vec3(
                    RandomFloat(-0.5f, 0.5f),
                    1.0f,
                    RandomFloat(-0.5f, 0.5f)
                ));
                float speed = RandomFloat(emitter.minStartSpeed, emitter.maxStartSpeed);

                p.velocity_size = glm::vec4(dir * speed, emitter.startSize);
                p.color = glm::vec4(emitter.startColor, 1.0f);

                m_newParticlesThisFrame.push_back(p);
            }
        }

        // --- RING BUFFER UPLOAD ---
        if (!m_newParticlesThisFrame.empty()) {
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_particleSSBO);

            size_t numToSpawn = m_newParticlesThisFrame.size();
            size_t spaceAtEnd = m_maxParticles - m_headIndex;

            if (numToSpawn <= spaceAtEnd) {
                glBufferSubData(GL_SHADER_STORAGE_BUFFER, m_headIndex * sizeof(ParticleData), numToSpawn * sizeof(ParticleData), m_newParticlesThisFrame.data());
                m_headIndex += numToSpawn;
            }
            else {
                glBufferSubData(GL_SHADER_STORAGE_BUFFER, m_headIndex * sizeof(ParticleData), spaceAtEnd * sizeof(ParticleData), m_newParticlesThisFrame.data());
                size_t remaining = numToSpawn - spaceAtEnd;
                glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, remaining * sizeof(ParticleData), m_newParticlesThisFrame.data() + spaceAtEnd);
                m_headIndex = remaining;
            }
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
        }
    }

    void ParticleSystem::DispatchCompute(float deltaTime) {

        std::shared_ptr<ShaderProgram> computeShader{};
        if (m_computeShaderGuid != xresource::full_guid{})
        {
            computeShader = EngineFramework::AssetManagerSystem::GetResource<ShaderProgram>(m_computeShaderGuid);
        }
        else
        {
            using E_AM = EngineFramework::AssetManagerSystem;
            m_computeShaderGuid = E_AM::GetFullGuid(E_AM::SHADER_GUID, "ParticleSimulation");
            computeShader = E_AM::GetResource<ShaderProgram>(m_computeShaderGuid);
        }

        if (!computeShader) {
			LOGE("Particle compute shader not found!");
            return;
        }

        computeShader->use();
        computeShader->setUniform("u_DeltaTime", deltaTime);
        computeShader->setUniform("u_MaxParticles", (int)m_maxParticles);

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_particleSSBO);

        GLuint numGroups = (static_cast<GLuint>(m_maxParticles) + 63) / 64;
        glDispatchCompute(numGroups, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);

        computeShader->disuse();
    }

    void ParticleSystem::RenderParticles(const glm::mat4& viewProj, const glm::mat4& viewMatrix) {

        std::shared_ptr<ShaderProgram> renderShader{};
        if (m_renderShaderGuid != xresource::full_guid{})
        {
            renderShader = EngineFramework::AssetManagerSystem::GetResource<ShaderProgram>(m_renderShaderGuid);
        }
        else
        {
            using E_AM = EngineFramework::AssetManagerSystem;
            m_renderShaderGuid = E_AM::GetFullGuid(E_AM::SHADER_GUID, "ParticleRender");
            renderShader = E_AM::GetResource<ShaderProgram>(m_renderShaderGuid);
        }

        if (!renderShader) {
			LOGE("Particle render shader not found!");
            return;
        } 

        // Spark Blending State
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE); // Additive blending for glow
        glDepthMask(GL_FALSE);             // Don't write to depth buffer

        renderShader->use();
        renderShader->setUniform("u_ViewProj", viewProj);
        renderShader->setUniform("u_View", viewMatrix);

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_particleSSBO);

        auto meshData = GraphicsSystem::GetInstance().GetMeshCache()["Quad"];

        // Standard instanced draw, no indirect buffer needed here
        glDrawElementsInstancedBaseVertex(
            GL_TRIANGLES, meshData->subMeshes[0].indexCount, GL_UNSIGNED_SHORT,
            (void*)(meshData->subMeshes[0].firstIndex * sizeof(Mesh::IndexType)),
            static_cast<GLsizei>(m_maxParticles), meshData->subMeshes[0].baseVertex
        );

        renderShader->disuse();
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
    }

    float ParticleSystem::RandomFloat(float min, float max) {
        static std::mt19937 rng(std::random_device{}());
        std::uniform_real_distribution<float> dist(min, max);
        return dist(rng);
    }
}