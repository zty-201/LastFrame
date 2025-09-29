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

// Keep this header small and self-contained
#include <string>
#include <unordered_map>
#include <functional>

#include "ISystem.hpp"

// FMOD forward declarations guarded by compile-time flag
#ifndef DISABLE_FMOD
#include "fmod.hpp"
#include "fmod_errors.h"
#include "SoundAsset.hpp"
#endif

namespace EngineFramework
{

    // High-level routing buckets for volumes/mutes etc.
    enum class AudioGroup
    {
        Master,
        Music,
        SFX
    };

    /**
     * AudioSystem
     * -----------
     * Minimal, engine-friendly wrapper around FMOD that:
     * - Initializes FMOD and creates channel groups (Master/Music/SFX)
     * - Loads sounds (by logical name or path) and plays them
     * - Lets the engine plug in a "path resolver" so you can map logical IDs -> real paths via AssetManager
     * - Supports Android asset manager (optional) by copying packed assets to a real file before FMOD loads them
     *
     * Notes:
     * - Linkage guarded by DISABLE_FMOD: you can compile stubs on platforms without FMOD.
     * - All public methods are stable/simple for gameplay code.
     */
    class AudioSystem final : public ISystem
    {
    public:
        // ---- ISystem lifecycle ----
        void Initialize() override;
        void Update(float delta_time) override;
        void PostUpdate(float delta_time) override;

        static AudioSystem& GetInstance() { return *this_system; }

        // ---- Engine integration ----
        // The engine (or AssetManager) can inject a resolver: logicalId -> absolute path
        void SetPathResolver(std::function<std::string(const std::string&)> resolver);

#ifdef PLATFORM_ANDROID
        // Provide the Android AAssetManager* (pass as void* to avoid android headers in the interface)
        void SetAndroidAssetManager(void* aasset_manager);
#endif

        // Allows quick tests without a resolver; if no resolver is set, logicalOrPath is treated as a literal path.
        std::string ResolvePath(const std::string& logicalOrPath) const;

        // ---- Public API for gameplay / systems ----
        
        //Load a sound from filepath.
        std::shared_ptr<SoundAsset> Load(const std::string& logicalOrPath, bool loop);

        // Unload a previously loaded sound.
        void Unload(const std::shared_ptr<SoundAsset>& sound);

        // Play a loaded sound; returns false if id not found or not initialized
        // volume: 0..1   paused: start paused if true   group: routing bucket
        bool Play(const std::shared_ptr<SoundAsset>& sound,
            float volume = 1.0f,
            bool paused = false,
            AudioGroup group = AudioGroup::SFX);

        // Global controls
        void StopAll();
        void PauseAll(bool paused);
        void SetMasterVolume(float volume);
        void SetGroupVolume(AudioGroup group, float volume);

        // Status
        bool IsInitialized() const;

        // ---- Ctor / Dtor ----
        AudioSystem();
        ~AudioSystem() override;

    private:
        // -------- Types / members --------

        // FMOD system + groups (null when not initialized or DISABLE_FMOD)
#ifndef DISABLE_FMOD
        FMOD::System* m_system;
        FMOD::ChannelGroup* m_groupMaster;
        FMOD::ChannelGroup* m_groupMusic;
        FMOD::ChannelGroup* m_groupSFX;
#else
        void* m_system;
        void* m_groupMaster;
        void* m_groupMusic;
        void* m_groupSFX;
#endif

        // Optional: engine-injected mapper logicalId -> path
        std::function<std::string(const std::string&)> m_pathResolver;

        bool m_initialized;

#ifdef PLATFORM_ANDROID
        // Stored as void* to avoid pulling in Android headers into this header
        void* m_androidAssetMgr;
#endif

        // -------- Internals --------
#ifndef DISABLE_FMOD
    // Log FMOD errors with context label
        void        logFmodError(FMOD_RESULT result, const char* context);
        // Choose FMOD mode flags (looped vs one-shot)
        FMOD_MODE   modeFor(bool looped);
#endif

        // Pick a channel group pointer (Master/Music/SFX)
        FMOD::ChannelGroup* pickGroup(AudioGroup group);

#ifdef PLATFORM_ANDROID
        // Copy an AAsset to a real file path (FMOD requires file paths on some platforms)
        std::string CopyAssetToInternal(const std::string& assetPath) const;
#endif

        static inline AudioSystem* this_system{};
    };

} // namespace EngineFramework

#endif // ENGINE_AUDIO_HPP
