/******************************************************************************/
/*!
\file       Audio.hpp
\author     Mohamed Arshad Bin Mohamed Hussain
\par        mohamedarshad.b\@digipen.edu
\brief      Declares the Audio System for the Cheerios Engine.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the prior
written consent of DigiPen Institute of Technology is prohibited.
*/
/******************************************************************************/
#pragma once
#ifndef ENGINE_AUDIO_HPP
#define ENGINE_AUDIO_HPP

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

#include "ISystem.hpp"

#ifndef DISABLE_FMOD
#include "fmod.hpp"
#include "fmod_errors.h"
#include "fmod_dsp.h"
#include "SoundAsset.hpp"
#endif

struct AudioComponent;
struct AudioListenerComponent;
struct TransformComponent;

namespace EngineFramework
{
    enum class AudioCategory : uint8_t
    {
        SFX = 0,
        BGM,
        UI,
        Ambience
    };

    enum class AudioGroup : uint8_t
    {
        Master = 0,
        BGM,
        SFX,
        UI,
        Ambience
    };

    enum class AudioRolloff
    {
        Linear,
        Inverse
    };

    class AudioSystem final : public ISystem
    {
    public:
        void Initialize() override;
        void Update(float delta_time) override;
        void PostUpdate(float delta_time) override;

        static AudioSystem& GetInstance() { return *this_system; }

        void SetPathResolver(std::function<std::string(const std::string&)> resolver);
        std::string ResolvePath(const std::string& logicalOrPath) const;

        // Play a named slot on an entity
        void PlayEntitySlot(entt::entity entity, const std::string& slotId);
        // Stop a named slot on an entity
        void StopEntitySlot(entt::entity entity, const std::string& slotId);
        // Stop all sounds start by this entity
        void StopAllForEntity(entt::entity entity);

        std::shared_ptr<SoundAsset> Load(const std::string& logicalOrPath, bool loop);

        bool Play(const std::shared_ptr<SoundAsset>& sound,
            float volume = 1.0f,
            bool paused = false,
            AudioGroup group = AudioGroup::SFX);

        bool PlayOneShot(const std::string& guid,
            const glm::vec3& worldPos,
            float volume = 1.0f,
            bool is3D = true,
            bool loop = false,
            float minDist = 1.0f,
            float maxDist = 50.0f,
            AudioGroup group = AudioGroup::SFX);

        void StopAll();
        void PauseAll(bool paused);

        void SetMasterVolume(float volume);
        void SetGroupVolume(AudioGroup group, float volume);

        bool IsInitialized() const;

        void SetSimulationMode(bool enabled);
        void SetIndoorFactor(float factor); // 0.0 = outdoor, 1.0 = indoor

        AudioSystem();
        ~AudioSystem() override;

    private:
        struct AudioCommand
        {
            enum class Type : uint8_t
            {
                PlaySlot,
                StopSlot,
                StopAllForEntity
            };

            Type type;
            entt::entity entity;
            std::string slotId;
        };

        struct ActiveInstance
        {
            entt::entity entity;
            std::string slotId;

#ifndef DISABLE_FMOD
            FMOD::Channel* channel = nullptr;
#endif
            // Fade feature
            bool useFade = false;
            float fadeDuration = 1.0f;
            float currentFadeVolume = 0.0f;
            float targetVolume = 1.0f;
            bool isFadingOut = false;

            // 3D feature
            bool is3D = false;
            bool looping = false;
            float minDistance = 1.0f;
            float maxDistance = 50.0f;

        };

        AudioGroup MapCategoryToGroup(AudioCategory category) const;

#ifndef DISABLE_FMOD
        void logFmodError(FMOD_RESULT result, const char* context);
        FMOD_MODE modeFor(bool looped, bool is3D, AudioRolloff rolloff) const;
        FMOD::ChannelGroup* pickGroup(AudioGroup group);
#endif

        // Commaned Processing
        void ProcessCommands(entt::registry& ecsRegistry);
        void ProcessPlaySlot(entt::registry& ecsRegistry, const AudioCommand& command);
        void ProcessStopSlot(const AudioCommand& command);
        void ProcessStopAllForEntity(const AudioCommand& command);

        void UpdateActiveInstances(entt::registry& ecsRegistry, float dt);
        void HandlePlayOnStart(entt::registry& ecsRegistry);

    private:
        float m_indoorFactor = 0.0f;

#ifndef DISABLE_FMOD
        FMOD::System* m_system = nullptr;

        FMOD::ChannelGroup* m_groupMaster = nullptr;
        FMOD::ChannelGroup* m_groupBGM = nullptr;
        FMOD::ChannelGroup* m_groupSFX = nullptr;
        FMOD::ChannelGroup* m_groupUI = nullptr;
        FMOD::ChannelGroup* m_groupAmbience = nullptr;
        FMOD::ChannelGroup* m_groupAmbienceOutdoor = nullptr;
        FMOD::ChannelGroup* m_groupAmbienceIndoor = nullptr;

        FMOD::DSP* m_ambienceLowpass = nullptr;
#else
        void* m_system = nullptr;
        void* m_groupMaster = nullptr;
        void* m_groupBGM = nullptr;
        void* m_groupSFX = nullptr;
        void* m_groupUI = nullptr;
        void* m_groupAmbience = nullptr;
        void* m_groupAmbienceOutdoor = nullptr;
        void* m_groupAmbienceIndoor = nullptr;
        void* m_ambienceLowpass = nullptr;
#endif

        std::function<std::string(const std::string&)> m_pathResolver;
        bool m_initialized = false;

#ifdef PLATFORM_ANDROID
        void* m_androidAssetManager = nullptr;
        std::string CopyAssetToInternal(const std::string& assetPath) const;
#endif

        // Runtime state
        std::vector<AudioCommand> m_commands;
        std::vector<ActiveInstance> m_active;

        std::unordered_map<entt::entity, bool> m_playOnStartDone;

        bool m_inSimulation = false;

        

        static AudioSystem* this_system;

    };

} // namespace EngineFramework

#endif // ENGINE_AUDIO_HPP
