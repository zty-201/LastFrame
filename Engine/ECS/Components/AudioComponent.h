#ifndef AUDIO_COMPONENT_H
#define AUDIO_COMPONENT_H

#include "xresource_guid.h"

#include <cstddef>
#include <vector>

struct AudioComponent
{
	struct AudioClip
	{
		// Which sound to play (linked to SoundAsset via this GUID)
		xresource::full_guid audio_guid{};

		// Basic playback controls
		float volume = 1.0f;
		bool loop = false;
		bool playOnStart = false;

		// 2D / 3D behaviour
		bool is3D = true;          // true = spatialized, false = pure 2D
		float minDistance = 1.0f;  // for 3D rolloff
		float maxDistance = 25.0f; // for 3D rolloff

		bool playRequested = false; // set by Lua / editor, consumed by AudioSystem
	};

	std::vector<AudioClip> Clips;

	AudioComponent()
		: Clips(1) {}

	std::size_t GetClipCount() const { return Clips.size(); }

	AudioClip* GetClip(std::size_t index)
	{
		if (index >= Clips.size())
			return nullptr;
		return &Clips[index];
	}

	const AudioClip* GetClip(std::size_t index) const
	{
		if (index >= Clips.size())
			return nullptr;
		return &Clips[index];
	}

	AudioClip& PrimaryClip()
	{
		if (Clips.empty())
			Clips.emplace_back();
		return Clips.front();
	}

	const AudioClip& PrimaryClip() const
	{
		if (Clips.empty())
		{
			static AudioClip defaultClip{};
			return defaultClip;
		}
		return Clips.front();
	}

	AudioClip& AddClip()
	{
		Clips.emplace_back();
		return Clips.back();
	}

	bool RemoveClip(std::size_t index)
	{
		if (Clips.size() <= 1 || index >= Clips.size())
			return false;

		Clips.erase(Clips.begin() + static_cast<std::vector<AudioClip>::difference_type>(index));
		return true;
	}
};

#endif // AUDIO_COMPONENT_H
