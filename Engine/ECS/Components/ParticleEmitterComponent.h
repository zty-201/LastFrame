/**
 * @file ParticleEmitterComponent.h
 * @brief Defines the configuration and spawning rules for a particle system attached to an entity.
 */
#ifndef PARTICLE_EMITTER_COMPONENT
#define PARTICLE_EMITTER_COMPONENT

struct ParticleEmitterComponent {
    bool isEmitting = true;
    float spawnRate = 50.0f;           // Particles per second
    float timeSinceLastSpawn = 0.0f;   // Internal timer

    glm::vec3 localOffset = { 0.0f, 0.0f, 0.0f };

    // Basic RNG Bounds
    float minLifetime = 0.5f;
    float maxLifetime = 1.5f;
    float minStartSpeed = 2.0f;
    float maxStartSpeed = 6.0f;
    float startSize = 0.2f;

    // Warm colors for sparks
    glm::vec3 startColor = { 1.0f, 0.6f, 0.1f };

    xresource::full_guid materialGuid{}; // To bind your spark texture
};

#endif