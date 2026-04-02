#ifndef PARTICLE_SYSTEM_H
#define PARTICLE_SYSTEM_H

#include "ISystem.hpp"
#include "ParticleEmitterComponent.h"
#include "TransformComponent.h"

namespace EngineFramework {

    // Matches the std430 layout in the Compute Shader perfectly
    struct ParticleData {
        glm::vec4 position_life; // xyz = pos, w = life
        glm::vec4 velocity_size; // xyz = vel, w = size
        glm::vec4 color;         // rgba
    };

    class ParticleSystem : public ISystem {
    public:
        ParticleSystem();
        ~ParticleSystem();

        void Initialize() override;
        void Update(float deltaTime) override;

        void DispatchCompute(float deltaTime);
        void RenderParticles(const glm::mat4& viewProj, const glm::mat4& viewMatrix);

    private:
        GLuint m_particleSSBO = 0;

        size_t m_maxParticles = 50000;
        size_t m_headIndex = 0;
        std::vector<ParticleData> m_newParticlesThisFrame;

        xresource::full_guid m_computeShaderGuid;
        xresource::full_guid m_renderShaderGuid;

        float RandomFloat(float min, float max);
    };
}
#endif