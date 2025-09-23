/******************************************************************************/
/*!
\file		Audio.cpp
\author 	Mohamed Arshad Bin Mohamed Hussain
\par    	mohamedarshad.b\@digipen.edu
\brief  	Audio System for the Cheerios Engine

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the prior
written consent of DigiPen Institute of Technology is prohibited.
*/
/******************************************************************************/
#include "pch.hpp"
#include "Audio.hpp"

#include <cassert>
#include <iostream>
#include <string>
#include <unordered_map>
#include <functional>
#include <filesystem>
#include <cstdio>

#ifdef PLATFORM_ANDROID
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#endif

namespace EngineFramework
{
    // ============================================================================
    // Internal helpers (FMOD)
    // ============================================================================
#ifndef DISABLE_FMOD
    static FMOD_VECTOR ToFmod(const Vec3& v) { FMOD_VECTOR o; o.x = v.x; o.y = v.y; o.z = v.z; return o; }

    void AudioSystem::logFmodError(FMOD_RESULT result, const char* context)
    {
        if (result != FMOD_OK)
        {
            std::cerr << "[AudioSystem] FMOD error in " << context
                << ": (" << static_cast<int>(result) << ") "
                << FMOD_ErrorString(result) << "\n";
        }
    }

    FMOD_MODE AudioSystem::modeFor(bool looped, bool is3D, AudioRolloff rolloff) const
    {
        FMOD_MODE mode = FMOD_DEFAULT;
        mode |= looped ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF;
        if (is3D)
        {
            mode |= FMOD_3D;
            mode |= (rolloff == AudioRolloff::Linear) ? FMOD_3D_LINEARROLLOFF : FMOD_3D_INVERSEROLLOFF;
        }
        return mode;
    }

    FMOD::ChannelGroup* AudioSystem::pickGroup(AudioGroup group)
    {
        switch (group)
        {
        case AudioGroup::Music:  return m_groupMusic;
        case AudioGroup::SFX:    return m_groupSFX;
        case AudioGroup::Master:
        default:                 return m_groupMaster;
        }
    }
#endif // !DISABLE_FMOD

    // ============================================================================
    // Constructor / Destructor
    // ============================================================================
    AudioSystem::AudioSystem() = default;

    AudioSystem::~AudioSystem()
    {
        releaseAllSounds();

#ifndef DISABLE_FMOD
        if (m_groupMusic) { m_groupMusic->release();  m_groupMusic = nullptr; }
        if (m_groupSFX) { m_groupSFX->release();    m_groupSFX = nullptr; }
        if (m_groupMaster) { m_groupMaster->release(); m_groupMaster = nullptr; }

        if (m_system)
        {
            m_system->close();
            m_system->release();
            m_system = nullptr;
        }
#endif
    }

    // ============================================================================
    // ISystem
    // ============================================================================
    void AudioSystem::Initialize()
    {
        if (m_initialized)
            return;

#ifndef DISABLE_FMOD
        FMOD_RESULT r;

        // 1) Create FMOD system
        r = FMOD::System_Create(&m_system);
        logFmodError(r, "System_Create");
        if (r != FMOD_OK || !m_system) return;

        // 2) Initialize FMOD
        r = m_system->init(64, FMOD_INIT_NORMAL, nullptr);
        logFmodError(r, "System::init");
        if (r != FMOD_OK) return;

        // 3) Create channel groups
        r = m_system->createChannelGroup("Master", &m_groupMaster);
        logFmodError(r, "createChannelGroup(Master)"); if (r != FMOD_OK) return;

        r = m_system->createChannelGroup("Music", &m_groupMusic);
        logFmodError(r, "createChannelGroup(Music)"); if (r != FMOD_OK) return;

        r = m_system->createChannelGroup("SFX", &m_groupSFX);
        logFmodError(r, "createChannelGroup(SFX)");   if (r != FMOD_OK) return;

        // 4) Connect child groups
        if (m_groupMaster && m_groupMusic) m_groupMaster->addGroup(m_groupMusic);
        if (m_groupMaster && m_groupSFX)   m_groupMaster->addGroup(m_groupSFX);

        // 5) Default volumes
        if (m_groupMaster) m_groupMaster->setVolume(1.0f);
        if (m_groupMusic)  m_groupMusic->setVolume(1.0f);
        if (m_groupSFX)    m_groupSFX->setVolume(1.0f);

        // 6) Apply initial 3D settings
        m_system->set3DSettings(m_dopplerScale, m_distanceFactor, m_rolloffScale);

        // 7) Initialize listener index
        m_listenerIndex = 0;
        FMOD_VECTOR pos = ToFmod(m_listenerPos);
        FMOD_VECTOR vel = ToFmod(m_listenerVel);
        FMOD_VECTOR fwd = ToFmod(m_listenerFwd);
        FMOD_VECTOR up = ToFmod(m_listenerUp);
        r = m_system->set3DListenerAttributes(m_listenerIndex, &pos, &vel, &fwd, &up);
        logFmodError(r, "set3DListenerAttributes");
#endif // !DISABLE_FMOD

        m_initialized = true;
        std::cout << "[AudioSystem] Initialization Successful!\n";
    }

    void AudioSystem::Update(float delta_time)
    {
        (void)delta_time;
#ifndef DISABLE_FMOD
        if (!m_initialized || !m_system)
            return;

        FMOD_VECTOR pos = ToFmod(m_listenerPos);
        FMOD_VECTOR vel = ToFmod(m_listenerVel);
        FMOD_VECTOR fwd = ToFmod(m_listenerFwd);
        FMOD_VECTOR up = ToFmod(m_listenerUp);
        FMOD_RESULT r = m_system->set3DListenerAttributes(m_listenerIndex, &pos, &vel, &fwd, &up);
        logFmodError(r, "set3DListenerAttributes (per-frame)");

        FMOD_RESULT r = m_system->update();
        logFmodError(r, "System::update");
#endif
    }

    // THIS IS EMPTY FOR NOW. MAYBE THERE WILL BE USE FOR IT IN THE FUTURE
    void AudioSystem::PostUpdate(float delta_time)
    {
        (void)delta_time;
    }

    // ============================================================================
    // Engine Integration
    // ============================================================================
    void AudioSystem::SetPathResolver(std::function<std::string(const std::string&)> resolver)
    {
        m_pathResolver = resolver;
    }

    std::string AudioSystem::ResolvePath(const std::string& logicalOrPath) const
    {
        if (m_pathResolver)
            return m_pathResolver(logicalOrPath);
        return logicalOrPath; // fallback for quick tests
    }

#ifdef PLATFORM_ANDROID
    void AudioSystem::SetAndroidAssetManager(void* aasset_manager)
    {
        m_androidAssetMgr = aasset_manager;
    }

    // If your platform code can pass a proper internal/data path, prefer that.
    // This fallback creates a subdir next to the executable for copies.
    static std::string GetInternalBaseDirFallback()
    {
        std::filesystem::path p = std::filesystem::current_path() / "audio_cache";
        std::error_code ec;
        std::filesystem::create_directories(p, ec);
        return p.string();
    }

    std::string AudioSystem::CopyAssetToInternal(const std::string& assetPath) const
    {
        if (m_androidAssetMgr == nullptr)
            return std::string();

        AAssetManager* mgr = reinterpret_cast<AAssetManager*>(m_androidAssetMgr);
        AAsset* asset = AAssetManager_open(mgr, assetPath.c_str(), AASSET_MODE_STREAMING);
        if (!asset)
        {
            std::cerr << "[AudioSystem][Android] AAsset not found: " << assetPath << "\n";
            return std::string();
        }

        const std::string base = GetInternalBaseDirFallback();
        const std::string dst = (std::filesystem::path(base) / assetPath).string();
        const std::string dir = std::filesystem::path(dst).parent_path().string();

        std::error_code ec;
        std::filesystem::create_directories(dir, ec);

        FILE* f = std::fopen(dst.c_str(), "wb");
        if (!f)
        {
            std::cerr << "[AudioSystem][Android] fopen failed for: " << dst << "\n";
            AAsset_close(asset);
            return std::string();
        }

        const void* buffer = AAsset_getBuffer(asset);
        const off_t length = AAsset_getLength(asset);
        bool ok = (buffer != nullptr && length > 0 &&
            std::fwrite(buffer, 1, static_cast<size_t>(length), f) == static_cast<size_t>(length));

        std::fclose(f);
        AAsset_close(asset);

        if (!ok)
        {
            std::cerr << "[AudioSystem][Android] write failed for: " << dst << "\n";
            return std::string();
        }

        return dst;
    }
#endif // PLATFORM_ANDROID

    // ============================================================================
    // Public API
    // ============================================================================
    bool AudioSystem::Load(const std::string& id, const std::string& logicalOrPath, bool loop)
    {
        if (!m_initialized)
            return false;

        if (m_sounds.find(id) != m_sounds.end())
            return true; // already loaded

        const std::string resolved = ResolvePath(logicalOrPath);

#ifndef DISABLE_FMOD
        FMOD::Sound* snd = nullptr;
        FMOD_MODE mode = modeFor(loop, false, AudioRolloff::Linear);

#ifdef PLATFORM_ANDROID
        // Example policy: add streaming on mobile for big files if desired
        // mode |= FMOD_CREATESTREAM;

        std::string loadPath = resolved;
        if (m_androidAssetMgr != nullptr)
        {
            std::string copied = CopyAssetToInternal(resolved);
            if (copied.empty()) return false;
            loadPath = copied;
        }

        FMOD_RESULT r = m_system->createSound(loadPath.c_str(), mode, nullptr, &snd);
#else
        FMOD_RESULT r = m_system->createSound(resolved.c_str(), mode, nullptr, &snd);
#endif
        logFmodError(r, "System::createSound(2D)");
        if (r != FMOD_OK || !snd)
            return false;

        m_sounds.emplace(id, SoundInfo{ snd, loop, false, AudioRolloff::Linear });
        return true;
#else
        (void)resolved; (void)loop;
        m_sounds.emplace(id, SoundInfo{ nullptr, loop, false, AudioRolloff::Linear });
        return true; // stub success
#endif
    }

    void AudioSystem::Unload(const std::string& id)
    {
        auto it = m_sounds.find(id);
        if (it == m_sounds.end()) return;

#ifndef DISABLE_FMOD
        if (it->second.sound)
        {
            it->second.sound->release();
            it->second.sound = nullptr;
        }
#endif
        m_sounds.erase(it);
    }

    bool AudioSystem::Play(const std::string& id, float volume, bool paused, AudioGroup group)
    {
        if (!m_initialized)
            return false;
        auto it = m_sounds.find(id);
        if (it == m_sounds.end())
            return false;

#ifndef DISABLE_FMOD
        if (!m_system || !it->second.sound)
            return false;

        FMOD::Channel* channel = nullptr;
        FMOD_RESULT r = m_system->playSound(it->second.sound, nullptr, true, &channel);
        logFmodError(r, "System::playSound(2D)");
        if (r != FMOD_OK || !channel)
            return false;

        if (FMOD::ChannelGroup* grp = pickGroup(group))
            channel->setChannelGroup(grp);

        if (volume < 0.0f) volume = 0.0f;
        if (volume > 1.0f) volume = 1.0f;
        channel->setVolume(volume);

        channel->setPaused(paused);
        if (!paused) channel->setPaused(false);

        return true;
#else
        (void)volume; (void)paused; (void)group;
        return true; // stub success
#endif
    }

    void AudioSystem::StopAll()
    {
        if (!m_initialized) return;
#ifndef DISABLE_FMOD
        if (m_groupMaster) m_groupMaster->stop();
#endif
    }

    void AudioSystem::PauseAll(bool paused)
    {
        if (!m_initialized) return;
#ifndef DISABLE_FMOD
        if (m_groupMaster) m_groupMaster->setPaused(paused);
#endif
    }

    void AudioSystem::SetMasterVolume(float volume)
    {
        if (!m_initialized) return;
        if (volume < 0.0f) volume = 0.0f;
        if (volume > 1.0f) volume = 1.0f;
#ifndef DISABLE_FMOD
        if (m_groupMaster) m_groupMaster->setVolume(volume);
#endif
    }

    void AudioSystem::SetGroupVolume(AudioGroup group, float volume)
    {
        if (!m_initialized) return;
        if (volume < 0.0f) volume = 0.0f;
        if (volume > 1.0f) volume = 1.0f;
#ifndef DISABLE_FMOD
        if (FMOD::ChannelGroup* grp = pickGroup(group))
            grp->setVolume(volume);
#endif
    }

    // ===== 3D Functions =====
    void AudioSystem::Set3DSettings(float dopplerScale, float distanceFactorMeters, float rolloffScale)
    {
        m_dopplerScale = dopplerScale;
        m_distanceFactor = distanceFactorMeters;
        m_rolloffScale = rolloffScale;

#ifndef DISABLE_FMOD
        if (m_initialized && m_system)
        {
            FMOD_RESULT r = m_system->set3DSettings(m_dopplerScale, m_distanceFactor, m_rolloffScale);
            logFmodError(r, "System::set3DSettings");
        }
#endif
    }

    void AudioSystem::SetListenerTransform(const Vec3& pos, const Vec3& vel, const Vec3& forward, const Vec3& up, int listenerIndex)
    {
        m_listenerPos = pos;
        m_listenerVel = vel;
        m_listenerFwd = forward;
        m_listenerUp = up;
        m_listenerIndex = listenerIndex;
    }

    bool AudioSystem::Load3D(const std::string& id, const std::string& logicalOrPath, bool loop, AudioRolloff rolloff)
    {
        if (!m_initialized) return false;
        if (m_sounds.find(id) != m_sounds.end()) return true;

        const std::string resolved = ResolvePath(logicalOrPath);

#ifndef DISABLE_FMOD
        FMOD::Sound* snd = nullptr;
        FMOD_MODE mode = modeFor(loop, true, rolloff);

#ifdef PLATFORM_ANDROID
        std::string loadPath = resolved;
        if (m_androidAssetMgr)
        {
            std::string copied = CopyAssetToInternal(resolved);
            if (copied.empty()) return false;
            loadPath = copied;
        }
        FMOD_RESULT r = m_system->createSound(loadPath.c_str(), mode, nullptr, &snd);
#else
        FMOD_RESULT r = m_system->createSound(resolved.c_str(), mode, nullptr, &snd);
#endif
        logFmodError(r, "System::createSound(3D)");
        if (r != FMOD_OK || !snd) return false;

        m_sounds.emplace(id, SoundInfo{ snd, loop, true, rolloff });
        return true;
#else
        (void)resolved; (void)loop; (void)rolloff;
        m_sounds.emplace(id, SoundInfo{ nullptr, loop, true, rolloff });
        return true;
#endif
    }

    void AudioSystem::SetDefault3DMinMax(float minDist, float maxDist)
    {
        if (minDist < 0.001f) minDist = 0.001f;
        if (maxDist < minDist) maxDist = minDist + 0.001f;
        m_defaultMinDist = minDist;
        m_defaultMaxDist = maxDist;
    }

    bool AudioSystem::Play3D(const std::string& id, const Vec3& pos, const Vec3& vel, float minDist, float maxDist, float volume, bool paused, AudioGroup group)
    {
        if (!m_initialized) return false;
        auto it = m_sounds.find(id);
        if (it == m_sounds.end()) return false;

#ifndef DISABLE_FMOD
        if (!m_system || !it->second.sound) return false;

        if (minDist <= 0.0f) minDist = m_defaultMinDist;
        if (maxDist <= 0.0f) maxDist = m_defaultMaxDist;

        FMOD::Channel* channel = nullptr;
        FMOD_RESULT r = m_system->playSound(it->second.sound, nullptr, true, &channel);
        logFmodError(r, "System::playSound(3D)");
        if (r != FMOD_OK || !channel) return false;

        if (FMOD::ChannelGroup* grp = pickGroup(group)) channel->setChannelGroup(grp);

        channel->setMode(FMOD_3D);

        FMOD_VECTOR position = ToFmod(pos);
        FMOD_VECTOR velocity = ToFmod(vel);
        channel->set3DAttributes(&position, &velocity);
        channel->set3DMinMaxDistance(minDist, maxDist);

        if (volume < 0.0f) volume = 0.0f;
        if (volume > 1.0f) volume = 1.0f;
        channel->setVolume(volume);

        channel->setPaused(paused);
        if (!paused) channel->setPaused(false);
        return true;
#else
        (void)pos; (void)vel; (void)minDist; (void)maxDist, (void)volume; (void)paused; (void)group;
        return true;
#endif
    }

    bool AudioSystem::IsInitialized() const
    {
        return m_initialized;
    }

    void AudioSystem::releaseAllSounds()
    {
#ifndef DISABLE_FMOD
        for (auto& keyVal : m_sounds)
        {
            if (keyVal.second.sound)
            {
                keyVal.second.sound->release();
                keyVal.second.sound = nullptr;
            }
        }
#endif

        m_sounds.clear();
    }

} // namespace EngineFramework
