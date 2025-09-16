#pragma once
#ifndef AUDIO_SYSTEM_HPP
#define AUDIO_SYSTEM_HPP

#include <string>
#include <unordered_map>

#include "fmod.h"
#include "fmod_errors.h"

#include "ISystem.hpp"

/*
* ----- Features of an Audio System -----
*   1. Initialize & Shutdown FMOD
*   2. Load audio file by name and file path
*   3. Play/Pause/Resume/Stop an audio
*   4. Set master volume, audio type volume (eg. SFX, Music, Ambient)
*   5. FMOD prints a readable error message (dont crash the engine)
*/

namespace EngineFramework
{
	enum class AudioGroup
	{
		Master,
		Music,
		SFX
	};

	class AudioSystem final : public ISystem
	{
	public:
		AudioSystem();
		~AudioSystem();

		// ===== ISystem =====
		void Initialize() override;
		void Update(float delta_time) override;

		bool Load(const std::string& id, const std::string& path, bool loop = false);
		void Unload(const std::string& id);
		bool Play(const std::string& id, float volume = 1.0f, bool paused = false, AudioGroup group = AudioGroup::SFX);

		void StopAll();
		void PauseAll(bool paused);

		void SetMasterVolume(float volume);
		void SetGroupVolume(AudioGroup group, float volume);

		bool IsInitialized() const;

	private:
		FMOD::System* m_system;
		FMOD::ChannelGroup* m_groupMaster;
		FMOD::ChannelGroup* m_groupMusic;
		FMOD::ChannelGroup* m_groupSFX;

		struct SoundInfo
		{
			FMOD::Sound* sound;
			bool looped;
		};
		std::unordered_map<std::string, SoundInfo> m_sounds;

		bool m_initialized;

		void releaseAllSounds();
		static void logFmodError(FMOD_RESULT result, const char* context);
		static FMOD_MODE modeFor(bool looped);
		FMOD::ChannelGroup* pickGroup(AudioGroup group);
	};
}
#endif // AUDIO_SYSTEM_HPP
