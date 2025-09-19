#pragma once
#ifndef AUDIO_HPP
#define AUDIO_HPP

#include <string>
#include <unordered_map>
#include <functional>

// Engine interface
#include "ISystem.hpp"

// ---------- FMOD includes / fallbacks ----------
// We still need the types for member pointers even if FMOD is disabled.
// So when DISABLE_FMOD is set, forward-declare the FMOD classes instead
// of including the headers.
#ifndef DISABLE_FMOD
#include <fmod.hpp>
#include <fmod_errors.h>
#else
namespace FMOD {
    class System;
    class ChannelGroup;
    class Sound;
    class Channel;
}
#endif

namespace EngineFramework
{
    enum class AudioGroup
    {
        Master,
        Music,
        SFX
    };

    // Minimal, cross-platform Audio System (FMOD Core API).
    class AudioSystem final : public ISystem
    {
    public:
        AudioSystem();
        ~AudioSystem();

        // ===== ISystem =====
        void Initialize() override;
        void Update(float delta_time) override;
        void PostUpdate(float delta_time) override;

        // ===== Engine Integration =====
        // Inject a path resolver from your Asset Manager. Given an id/logical name,
        // return a platform-ready filesystem path.
        void SetPathResolver(std::function<std::string(const std::string&)> resolver);

#ifdef PLATFORM_ANDROID
        // Provide Android AAssetManager* (as void* to avoid including NDK headers in the public header).
        void SetAndroidAssetManager(void* aasset_manager);
#endif

        // ===== Public API =====
        bool Load(const std::string& id, const std::string& logicalOrPath, bool loop = false);
        void Unload(const std::string& id);

        bool Play(const std::string& id,
            float volume = 1.0f,
            bool paused = false,
            AudioGroup group = AudioGroup::SFX);

        void StopAll();
        void PauseAll(bool paused);

        void SetMasterVolume(float volume);
        void SetGroupVolume(AudioGroup group, float volume);

        bool IsInitialized() const;

    private:
        // -------- FMOD state --------
        FMOD::System* m_system;
        FMOD::ChannelGroup* m_groupMaster;
        FMOD::ChannelGroup* m_groupMusic;
        FMOD::ChannelGroup* m_groupSFX;

        struct SoundInfo
        {
            FMOD::Sound* sound;
            bool         looped;
        };
        std::unordered_map<std::string, SoundInfo> m_sounds;

        bool m_initialized;

        // -------- Integration hooks --------
        std::function<std::string(const std::string&)> m_pathResolver;

#ifdef PLATFORM_ANDROID
        void* m_androidAssetMgr; // AAssetManager*
        std::string CopyAssetToInternal(const std::string& assetPath) const;
#endif

        // -------- Helpers --------
        std::string ResolvePath(const std::string& logicalOrPath) const;

#ifndef DISABLE_FMOD
        static void      logFmodError(FMOD_RESULT result, const char* context);
        static FMOD_MODE modeFor(bool looped);
#endif
        FMOD::ChannelGroup* pickGroup(AudioGroup group);
        void                releaseAllSounds();
    };
}

#endif // AUDIO_HPP
