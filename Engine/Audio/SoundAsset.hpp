/*
	Defines a sound asset that follows RAII Rule of 3.
*/

#ifndef SOUND_ASSET_HPP
#define SOUND_ASSET_HPP

#include "fmod.hpp"
#include "fmod_errors.h"

class SoundAsset
{
public:
	SoundAsset() = default;
	~SoundAsset()
	{
		if (sound) sound->release();
		sound = nullptr;
	}

	//Rule of 3.
	SoundAsset& operator=(const SoundAsset& rhs) = delete;
	SoundAsset(const SoundAsset& rhs) = delete;

	FMOD::Sound* sound{};
	bool looped{};
};



#endif