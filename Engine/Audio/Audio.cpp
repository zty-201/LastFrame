#include "pch.hpp"
#include "Audio.hpp"

#include <cassert>
#include <iostream>

namespace EngineFramework
{
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
		return looped ? (FMOD_DEFAULT | FMOD_LOOP_NORMAL) : (FMOD_DEFAULT | FMOD_LOOP_OFF);
	}

	FMOD::ChannelGroup* AudioSystem::pickGroup(AudioGroup group)
	{
		switch (group)
		{
			case AudioGroup::Music:		return m_groupMusic;
			case AudioGroup::SFX:		return m_groupSFX;

			case AudioGroup::Master:
			default:					return m_groupMaster;
		}
	}

	AudioSystem::AudioSystem()
		: m_system(nullptr)
		, m_groupMaster(nullptr)
		, m_groupMusic(nullptr)
		, m_groupSFX(nullptr)
		, m_initialized(false)
	{
	}

	AudioSystem::~AudioSystem()
	{
		releaseAllSounds();

		if (m_groupMusic)
		{
			m_groupMusic->release();
			m_groupMusic = nullptr;
		}
		if (m_groupSFX)
		{
			m_groupSFX->release();
			m_groupSFX = nullptr;
		}
		if (m_groupMaster)
		{
			m_groupMaster->release();
			m_groupMaster = nullptr;
		}

		if (m_system)
		{
			m_system->close();
			m_system->release();
			m_system = nullptr;
		}
	}

	void AudioSystem::Initialize()
	{
		if (m_initialized)
			return;

		FMOD_RESULT r;

		// Step 1: Create FMOD System
		r = FMOD::System_Create(&m_system);
		logFmodError(r, "System_Create");
		if (r != FMOD_OK || m_system == nullptr) return;

		// Step 2: Initialize maximum amount of channels (start with 64, increase later if needed)
		r = m_system->init(64, FMOD_INIT_NORMAL, nullptr);
		logFmodError(r, "System::init");
		if (r != FMOD_OK) return;

		// Step 3: Create channel groups
		r = m_system->createChannelGroup("Master", &m_groupMaster);
		logFmodError(r, "createChannelGroup(Master)");
		if (r != FMOD_OK) return;

		r = m_system->createChannelGroup("Music", &m_groupMusic);
		logFmodError(r, "createChannelGroup(Music)");
		if (r != FMOD_OK) return;

		r = m_system->createChannelGroup("SFX", &m_groupSFX);
		logFmodError(r, "createChannelGroup(SFX)");
		if (r != FMOD_OK) return;

		// Step 4: Wire child groups under Master
		if (m_groupMaster && m_groupMusic) m_groupMaster->addGroup(m_groupMusic);
		if (m_groupMaster && m_groupSFX) m_groupMaster->addGroup(m_groupSFX);

		// Step 5: Default volumes
		if (m_groupMaster) m_groupMaster->setVolume(1.0f);
		if (m_groupMusic) m_groupMusic->setVolume(1.0f);
		if (m_groupSFX) m_groupSFX->setVolume(1.0f);

		m_initialized = true;
		std::cout << "[AudioSystem] Initialization Successful!\n";
	}

	void AudioSystem::Update(float delta_time)
	{
		(void)delta_time;
		if (!m_initialized || m_system == nullptr)
			return;

		FMOD_RESULT r = m_system->update();
		logFmodError(r, "System::update");
	}

	bool AudioSystem::Load(const std::string& id, const std::string& path, bool loop)
	{
		if (!m_initialized || m_system = nullptr)
			return false;

		if (m_sounds.find(id) != m_sounds.end())
			return true;

		FMOD::Sound* snd = nullptr;
		FMOD_MODE mode = modeFor(loop);

		// For short SFX use normal (fully decoded) load; for long music later, use FMOD_CREATESTREAM.
		FMOD_RESULT r = m_system->createSound(path.c_str(), mode, nullptr, &snd);
		logFmodError(r, "System::createSound");
		if (r != FMOD_OK || snd == nullptr)
			return false;

		SoundInfo info;
		info.sound = snd;
		info.looped = loop;
		m_sounds.emplace(id, info);
		return true;
	}

	void AudioSystem::Unload(const std::string& id)
	{
		std::unordered_map<std::string, SoundInfo>::iterator it = m_sounds.find(id);
		if (it == m_sounds.end()) return;

		if (it->second.sound)
		{
			it->second.sound->release();
			it->second.sound = nullptr;
		}

		m_sounds.erase(it);
	}

	bool AudioSystem::Play(const std::string& id, float volume, bool paused, AudioGroup group)
	{
		if (!m_initialized || m_system == nullptr) return false;

		std::unordered_map<std::string, SoundInfo>::const_iterator it = m_sounds.find(id);
		if (it == m_sounds.end() || it->second.sound == nullptr) return false;

		FMOD::Channel* channel = nullptr;
		FMOD_RESULT r = m_system->playSound(it->second.sound, nullptr, true, &channel);
		logFmodError(r, "System::playSound");
		if (r != FMOD_OK || channel == nullptr) return false;

		// Route to correct group
		FMOD::ChannelGroup* grp = pickGroup(group);
		if (grp)
		{
			channel->setChannelGroup(grp);
		}

		// Set initial volume (clamp 0..1 gently)
		if (volume < 0.0f) volume = 0.0f;
		if (volume > 1.0f) volume = 1.0f;
		channel->setVolume(volume);

		// Apply paused flag (we started paused = true to avoid pops while configuring)
		channel->setPaused(paused);

		// Finally unpause to actually start if requested
		if (!paused)
		{
			channel->setPaused(false);
		}

		return true;
	}

	void AudioSystem::StopAll()
	{
		if (!m_initialized) return;
		if (m_groupMaster) m_groupMaster->stop();
	}

	void AudioSystem::PauseAll(bool paused)
	{
		if (!m_initialized) return;
		if (m_groupMaster) m_groupMaster->setPaused(paused);
	}

	void AudioSystem::SetMasterVolume(float volume)
	{
		if (!m_initialized) return;
		if (volume < 0.0f) volume = 0.0f;
		if (volume > 1.0f) volume = 1.0f;
		if (m_groupMaster) m_groupMaster->setVolume(volume);
	}

	void AudioSystem::SetGroupVolume(AudioGroup group, float volume)
	{
		if (!m_initialized) return;
		if (volume < 0.0f) volume = 0.0f;
		if (volume > 1.0f) volume = 1.0f;

		FMOD::ChannelGroup* grp = pickGroup(group);
		if (grp) grp->setVolume(volume);
	}

	bool AudioSystem::IsInitialized() const
	{
		return m_initialized;
	}

	void AudioSystem::releaseAllSounds()
	{
		for (std::unordered_map<std::string, SoundInfo>::iterator it = m_sounds.begin(); it != m_sounds.end(); ++it)
		{
			if (it->second.sound)
			{
				it->second.sound->release();
				it->second.sound = nullptr;
			}
		}
		m_sounds.clear();
	}
}