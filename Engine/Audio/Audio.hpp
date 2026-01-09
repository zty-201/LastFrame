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

#include <string>
#include <unordered_map>
#include <functional>

#include "ISystem.hpp"

#ifndef DISABLE_FMOD
#include "fmod.hpp"
#include "fmod_errors.h"
#include "SoundAsset.hpp"
#endif

struct AudioComponent;
struct AudioListenerComponent;
struct TransformComponent;

namespace EngineFramework
{
    // For listener/source transform positions
    struct Vec3
    {
        float x, y, z;
        Vec3() : x(0), y(0), z(0){}
        Vec3(float X, float Y, float Z) : x(X), y(Y), z(Z) {}
    };

    // Different types of audio
    enum class AudioGroup
    {
        Master,
        Music,
        SFX
    };

    // For audio to fade-out realistically
    enum class AudioRolloff
    {
        Linear,
        Inverse // Mostly preferred in a 3D environment
    };

    /**
     * AudioSystem
     * -----------
     * 1) Initializes FMOD and creates channel groups (Master/Music/SFX)
     * 
     * 2) Loads sounds (by logical name or path)
     *  - Eg. Load("lizard", "lizard.wav")
     * 
     * 3) Play/Pause/Stop sounds when called
     *  - Eg. Play("lizard")
     * 
     * 4) Adjust volumes
     *
     * Notes:
     * - Linkage guarded by DISABLE_FMOD: you can compile stubs on platforms without FMOD.
     * - All public methods are stable/simple for gameplay code.
     */
    class AudioSystem final : public ISystem
    {
    public:

        void Initialize() override;
        void Update(float delta_time) override;
        void PostUpdate(float delta_time) override;

        static AudioSystem& GetInstance() { return *this_system; }

        // ---- Engine integration ----
        // The engine (or AssetManager) can inject a resolver: logicalId -> absolute path
        void SetPathResolver(std::function<std::string(const std::string&)> resolver);
        // Allows quick tests without a resolver; if no resolver is set, logicalOrPath is treated as a literal path.
        std::string ResolvePath(const std::string& logicalOrPath) const;

#ifdef PLATFORM_ANDROID
        // Provide the Android AAssetManager* (pass as void* to avoid android headers in the interface)
        void SetAndroidAssetManager(void* aasset_manager);
#endif


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

        /*bool PlayFromComponent(const AudioComponent& comp,
            const glm::vec3& worldPos,
            AudioGroup group = AudioGroup::SFX);*/
        bool PlayOneShot(const std::string& guid,
            const glm::vec3& worldPos,
            float volume = 1.0f,
            bool is3D = true,
            bool loop = false,
            float minDist = 1.0f,
            float maxDist = 50.0f,
            AudioGroup group = AudioGroup::SFX);

        // ===== 3D Functions =====
        // 3D Configuration
        //void Set3DSettings(float dopplerScale = 1.0f,
        //    float distanceFactorMeters = 1.0f,
        //    float rolloffScale = 1.0f);

        //// Call this during Update() to follow player/camera
        //void SetListenerTransform(const Vec3& pos,
        //    const Vec3& forward,
        //    const Vec3& up,
        //    const Vec3& velocity = Vec3(0, 0, 0),
        //    int listenerIndex = 0);

        //// Specifically to load 3D audio
        //bool Load3D(const std::string& id,
        //    const std::string& logicalOrPath,
        //    bool loop = false,
        //    AudioRolloff rolloff = AudioRolloff::Linear);

        //bool Play3D(const std::string& id,
        //    const Vec3& pos,
        //    const Vec3& vel = Vec3(0, 0, 0),
        //    float minDist = 1.0f,
        //    float maxDist = 50.0f,
        //    float volume = 1.0f,
        //    bool paused = false,
        //    AudioGroup group = AudioGroup::SFX);

        //void SetDefault3DMinMax(float minDist, float maxDist);

        // Global controls
        void StopAll();
        void PauseAll(bool paused);
        void SetMasterVolume(float volume);
        void SetGroupVolume(AudioGroup group, float volume);

        // Request Status
        bool IsInitialized() const;

        //void DebugPlayOnKey(int key, const std::shared_ptr<SoundAsset>& sound);

        // Constructor / Destructor
        AudioSystem();
        ~AudioSystem() override;

    private:
        // -------- Types / members --------

        // FMOD system + groups (null when not initialized or DISABLE_FMOD)
#ifndef DISABLE_FMOD
        FMOD::System*       m_system        = nullptr;
        FMOD::ChannelGroup* m_groupMaster   = nullptr;
        FMOD::ChannelGroup* m_groupMusic    = nullptr;
        FMOD::ChannelGroup* m_groupSFX      = nullptr;
#else
        void* m_system          = nullptr;
        void* m_groupMaster     = nullptr;
        void* m_groupMusic      = nullptr;
        void* m_groupSFX        = nullptr;
#endif

        // Optional: engine-injected mapper logicalId -> path
        std::function<std::string(const std::string&)> m_pathResolver;

        bool m_initialized = false;

        // 3D global settings
        float m_dopplerScale   = 1.0f;
        float m_distanceFactor = 1.0f;
        float m_rolloffScale   = 1.0f;

        // 3D listener state
        Vec3 m_listenerPos = Vec3(0, 0, 0);
        Vec3 m_listenerVel = Vec3(0, 0, 0);
        Vec3 m_listenerFwd = Vec3(0, 0, -1);
        Vec3 m_listenerUp  = Vec3(0, 1, 0);
        int m_listenerIndex = 0;

        // Default attenuation for Play3D
        float m_defaultMinDist = 1.0f;
        float m_defaultMaxDist = 50.0f;

#ifdef PLATFORM_ANDROID
        // Stored as void* to avoid pulling in Android headers into this header
        void* m_androidAssetMgr;
        // Copy an AAsset to a real file path (FMOD requires file paths on some platforms)
        std::string CopyAssetToInternal(const std::string& assetPath) const;
#endif

        // -------- Internals --------
#ifndef DISABLE_FMOD
    // Log FMOD errors with context label
        void        logFmodError(FMOD_RESULT result, const char* context);
        // Choose FMOD mode flags (looped vs one-shot)
        FMOD_MODE   modeFor(bool looped, bool is3D, AudioRolloff rolloff) const;
        // Pick a channel group pointer (Master/Music/SFX)
        FMOD::ChannelGroup* pickGroup(AudioGroup group);

        // Testing purpose
        //void DebugPlayTestSound();
        // 
        void DebugStereoPanTest();

        void DebugMainMenuAutoPlay();
        void DebugMainMenuHandleKeys();
        bool m_debugMainMenuStarted = false;
#endif

        static inline AudioSystem* this_system{};
    };

} // namespace EngineFramework

#endif // ENGINE_AUDIO_HPP
