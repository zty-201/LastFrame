#ifndef AUDIO_COMPONENT_H
#define AUDIO_COMPONENT_H

#include "xresource_guid.h"

struct AudioComponent
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

#endif // AUDIO_COMPONENT_H
