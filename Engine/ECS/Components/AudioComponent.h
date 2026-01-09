#pragma once
#include <string>
#include <vector>
#include <unordered_map>
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

// A request to play a specific sound
struct AudioRequest
{
    std::string clipName;   // The logical name (e.g. "Footstep")
    float volume = 1.0f;
    bool is3D = true;
    bool loop = false;
    float minDistance = 1.0f;
    float maxDistance = 50.0f;
};

struct AudioComponent
{
    // 1. CONFIG: Map "Logical Name" -> "File Path" (or GUID)
    std::unordered_map<std::string, std::string> soundBank;

    // 2. STATE: The list of sounds to play this frame
    std::vector<AudioRequest> requestQueue;

    // 3. PLAY ON START CONFIGURATION (Add these lines!)
    std::string playOnStartName;  // Name of the sound to play (e.g. "BGM")
    bool playOnStartLoop = true;
    bool playOnStart3D = false;
    float playOnStartVolume = 0.5f;

    // 4. RUNTIME STATE
    bool _hasStarted = false; // Prevents it from restarting every frame

    // 5. HELPER
    void Play(const std::string& name, float vol = 1.0f)
    {
        AudioRequest req;
        req.clipName = name;
        req.volume = vol;
        req.is3D = true;
        req.loop = false;
        requestQueue.push_back(req);
    }
};