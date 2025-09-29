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
		// For short SFX we keep fully decoded. For long music you could add FMOD_CREATESTREAM.
		return looped ? (FMOD_DEFAULT | FMOD_LOOP_NORMAL)
			: (FMOD_DEFAULT | FMOD_LOOP_OFF);
	}
#endif // !DISABLE_FMOD

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

	// ============================================================================
	// Ctor / Dtor
	// ============================================================================
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
		this_system = this;
	}

	AudioSystem::~AudioSystem()
	{
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
		if (r != FMOD_OK || m_system == NULL) return;

		// 2) Initialize FMOD
		r = m_system->init(64, FMOD_INIT_NORMAL, NULL);
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
#endif // !DISABLE_FMOD

		m_initialized = true;
		LOGI("%s", "[AudioSystem] Initialization Successful!");
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

	// ============================================================================
	// Engine Integration
	// ============================================================================
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

	std::string AudioSystem::ResolvePath(const std::string& logicalOrPath) const
	{
		if (m_pathResolver)
			return m_pathResolver(logicalOrPath);
		return logicalOrPath; // fallback for quick tests
	}

#ifdef PLATFORM_ANDROID
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
#endif // PLATFORM_ANDROID

	// ============================================================================
	// Public API
	// ============================================================================
	std::shared_ptr<SoundAsset> AudioSystem::Load(const std::string& logicalOrPath, bool loop)
	{
		if (!m_initialized)
			throw std::exception("Audio System Not Initialized");

		const std::string resolved = ResolvePath(logicalOrPath);

#ifndef DISABLE_FMOD
		FMOD::Sound* snd = NULL;
		FMOD_MODE mode = modeFor(loop);

#ifdef PLATFORM_ANDROID
		// Example policy: add streaming on mobile for big files if desired
		// mode |= FMOD_CREATESTREAM;

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
			throw std::exception("Audio Not Loaded");

		std::shared_ptr<SoundAsset> info = std::make_shared<SoundAsset>();
		info->sound = snd;
		info->looped = loop;
		return info;
#else
		(void)resolved; (void)loop;
		m_sounds.insert(std::make_pair(id, SoundInfo{ NULL, loop }));
		return true; // stub success
#endif
	}

	void AudioSystem::Unload(const std::shared_ptr<SoundAsset>& sound)
	{
		if (sound->sound) sound->sound->release();
		sound->sound = nullptr;
	}

	bool AudioSystem::Play(const std::shared_ptr<SoundAsset>& sound, float volume, bool paused, AudioGroup group)
	{
		if (!m_initialized)
			return false;

#ifndef DISABLE_FMOD
		if (m_system == NULL || sound->sound == NULL)
			return false;

		FMOD::Channel* channel = NULL;
		FMOD_RESULT r = m_system->playSound(sound->sound, NULL, true, &channel);
		logFmodError(r, "System::playSound");
		if (r != FMOD_OK || channel == NULL)
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

	bool AudioSystem::IsInitialized() const
	{
		return m_initialized;
	}
} // namespace EngineFramework
