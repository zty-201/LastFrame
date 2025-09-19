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
    // =======================
    // Internal helpers (FMOD)
    // =======================
#ifndef DISABLE_FMOD
    void AudioSystem::logFmodError(FMOD_RESULT result, const char* context)
    {
        if (result != FMOD_OK)
        {
            std::cerr << "[AudioSystem] FMOD error in " << context
                << ": (" << static_cast<int>(result) << ") "
                << FMOD_ErrorString(result) << "\n";
        }
    }

    FMOD_MODE AudioSystem::modeFor(bool looped)
    {
        // For SFX we keep fully loaded; for long music consider | FMOD_CREATESTREAM
        return looped ? (FMOD_DEFAULT | FMOD_LOOP_NORMAL) : (FMOD_DEFAULT | FMOD_LOOP_OFF);
    }
#endif

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

    // =======================
    // Ctor / Dtor
    // =======================
    AudioSystem::AudioSystem()
        : m_system(NULL)
        , m_groupMaster(NULL)
        , m_groupMusic(NULL)
        , m_groupSFX(NULL)
        , m_initialized(false)

#ifdef PLATFORM_ANDROID
        , m_androidAssetMgr(NULL)
#endif

    {
    }

    AudioSystem::~AudioSystem()
    {
        releaseAllSounds();

#ifndef DISABLE_FMOD
        if (m_groupMusic) { m_groupMusic->release();  m_groupMusic = NULL; }
        if (m_groupSFX) { m_groupSFX->release();    m_groupSFX = NULL; }
        if (m_groupMaster) { m_groupMaster->release(); m_groupMaster = NULL; }

        if (m_system)
        {
            m_system->close();
            m_system->release();
            m_system = NULL;
        }
#endif
    }

    // =======================
    // ISystem
    // =======================
    void AudioSystem::Initialize()
    {
        if (m_initialized)
            return;

#ifndef DISABLE_FMOD
        FMOD_RESULT r;

        // 1) Create FMOD system
        r = FMOD::System_Create(&m_system);
        logFmodError(r, "System_Create");
        if (r != FMOD_OK || m_system == NULL) return;

        // 2) Init FMOD
        r = m_system->init(64, FMOD_INIT_NORMAL, NULL);
        logFmodError(r, "System::init");
        if (r != FMOD_OK) return;

        // 3) Channel groups
        r = m_system->createChannelGroup("Master", &m_groupMaster);
        logFmodError(r, "createChannelGroup(Master)"); if (r != FMOD_OK) return;

        r = m_system->createChannelGroup("Music", &m_groupMusic);
        logFmodError(r, "createChannelGroup(Music)"); if (r != FMOD_OK) return;

        r = m_system->createChannelGroup("SFX", &m_groupSFX);
        logFmodError(r, "createChannelGroup(SFX)");   if (r != FMOD_OK) return;

        if (m_groupMaster && m_groupMusic) m_groupMaster->addGroup(m_groupMusic);
        if (m_groupMaster && m_groupSFX)   m_groupMaster->addGroup(m_groupSFX);

        if (m_groupMaster) m_groupMaster->setVolume(1.0f);
        if (m_groupMusic)  m_groupMusic->setVolume(1.0f);
        if (m_groupSFX)    m_groupSFX->setVolume(1.0f);
#endif

        m_initialized = true;
        std::cout << "[AudioSystem] Initialization Successful!\n";
    }

    void AudioSystem::Update(float delta_time)
    {
        (void)delta_time;
#ifndef DISABLE_FMOD
        if (!m_initialized || m_system == NULL)
            return;

        FMOD_RESULT r = m_system->update();
        logFmodError(r, "System::update");
#endif
    }

    void AudioSystem::PostUpdate(float delta_time)
    {
        (void)delta_time;
    }

    // =======================
    // Engine Integration
    // =======================
    void AudioSystem::SetPathResolver(std::function<std::string(const std::string&)> resolver)
    {
        m_pathResolver = resolver;
    }

#ifdef PLATFORM_ANDROID
    void AudioSystem::SetAndroidAssetManager(void* aasset_manager)
    {
        m_androidAssetMgr = aasset_manager;
    }
#endif

    // Logical to real path
    std::string AudioSystem::ResolvePath(const std::string& logicalOrPath) const
    {
        if (m_pathResolver)
            return m_pathResolver(logicalOrPath);
        return logicalOrPath; // fallback: treat as literal path (useful for quick tests)
    }

#ifdef PLATFORM_ANDROID
    // NOTE: Ideally your platform layer provides an app-writable files directory path.
    // If not, this fallback uses a simple subfolder "audio_cache" within the process's CWD.
    static std::string GetInternalBaseDirFallback()
    {
        // You can replace this with a path passed down from your Android app glue.
        std::filesystem::path p = std::filesystem::current_path() / "audio_cache";
        std::error_code ec; std::filesystem::create_directories(p, ec);
        return p.string();
    }

    std::string AudioSystem::CopyAssetToInternal(const std::string& assetPath) const
    {
        if (m_androidAssetMgr == NULL)
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
        bool ok = (buffer != NULL && length > 0 &&
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
#endif

    // =======================
    // Public API
    // =======================
    bool AudioSystem::Load(const std::string& id, const std::string& logicalOrPath, bool loop)
    {
        if (!m_initialized)
            return false;

        if (m_sounds.find(id) != m_sounds.end())
            return true;

        const std::string resolved = ResolvePath(logicalOrPath);

#ifndef DISABLE_FMOD
        FMOD::Sound* snd = NULL;
        FMOD_MODE mode = modeFor(loop);

#ifdef PLATFORM_ANDROID
        // On mobile, large assets often stream better; flip this on per policy if desired.
        // mode |= FMOD_CREATESTREAM;

        // If assets are packaged in APK, copy to a real file path first.
        std::string loadPath = resolved;
        if (m_androidAssetMgr != NULL)
        {
            std::string copied = CopyAssetToInternal(resolved);
            if (copied.empty()) return false;
            loadPath = copied;
        }

        FMOD_RESULT r = m_system->createSound(loadPath.c_str(), mode, NULL, &snd);
#else
        FMOD_RESULT r = m_system->createSound(resolved.c_str(), mode, NULL, &snd);
#endif
        logFmodError(r, "System::createSound");
        if (r != FMOD_OK || snd == NULL)
            return false;

        SoundInfo info;
        info.sound = snd;
        info.looped = loop;
        m_sounds.insert(std::make_pair(id, info));
        return true;
#else
        (void)resolved; (void)loop;
        // Stub success when FMOD is disabled
        m_sounds.insert(std::make_pair(id, SoundInfo{ NULL, loop }));
        return true;
#endif
    }

    void AudioSystem::Unload(const std::string& id)
    {
        std::unordered_map<std::string, SoundInfo>::iterator it = m_sounds.find(id);
        if (it == m_sounds.end()) return;

#ifndef DISABLE_FMOD
        if (it->second.sound)
        {
            it->second.sound->release();
            it->second.sound = NULL;
        }
#endif
        m_sounds.erase(it);
    }

    bool AudioSystem::Play(const std::string& id, float volume, bool paused, AudioGroup group)
    {
        if (!m_initialized)
            return false;

        std::unordered_map<std::string, SoundInfo>::const_iterator it = m_sounds.find(id);
        if (it == m_sounds.end())
            return false;

#ifndef DISABLE_FMOD
        if (m_system == NULL || it->second.sound == NULL)
            return false;

        FMOD::Channel* channel = NULL;
        FMOD_RESULT r = m_system->playSound(it->second.sound, NULL, true, &channel);
        logFmodError(r, "System::playSound");
        if (r != FMOD_OK || channel == NULL)
            return false;

        FMOD::ChannelGroup* grp = pickGroup(group);
        if (grp) { channel->setChannelGroup(grp); }

        if (volume < 0.0f) volume = 0.0f;
        if (volume > 1.0f) volume = 1.0f;
        channel->setVolume(volume);

        channel->setPaused(paused);
        if (!paused) { channel->setPaused(false); }

        return true;
#else
        (void)volume; (void)paused; (void)group;
        return true; // Stub success without FMOD
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
        FMOD::ChannelGroup* grp = pickGroup(group);
        if (grp) grp->setVolume(volume);
#endif
    }

    bool AudioSystem::IsInitialized() const
    {
        return m_initialized;
    }

    void AudioSystem::releaseAllSounds()
    {
        std::unordered_map<std::string, SoundInfo>::iterator it = m_sounds.begin();
        for (; it != m_sounds.end(); ++it)
        {
#ifndef DISABLE_FMOD
            if (it->second.sound)
            {
                it->second.sound->release();
                it->second.sound = NULL;
            }
#endif
        }
        m_sounds.clear();
    }
}
