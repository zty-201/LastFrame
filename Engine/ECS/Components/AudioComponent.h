#ifndef AUDIO_COMPONENT_H
#define AUDIO_COMPONENT_H

#include "xresource_guid.h"

struct AudioComponent
{
	xresource::full_guid audio_guid{};
	float volume = 1.0f;
	bool loop = false;
	bool playOnStart = false;
};

#endif // AUDIO_COMPONENT_H
