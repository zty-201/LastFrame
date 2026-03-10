#pragma once

#include "xresource_guid.h"
#include "Audio.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

struct AudioComponent
{

	// Explicit spatial mode for readability
	enum class SpatialMode : uint8_t
	{
		TwoD = 0,
		ThreeD
	};

	enum class AmbienceType : uint8_t
	{
		General = 0,
		Outdoor,
		Indoor
	};

	using AudioCategory = EngineFramework::AudioCategory;

	// A 'Slot' is a designer-authored "audio" in an entity
	/* 
	* Eg. "Pickup", "Footstep"
	* In LUA, call for AudioSystem like:
	* - Audio.Play(entity."Pickup")
	* - Audio.Play(entity."Footstep")
	*/
	struct Slot
	{
		// Identifiers used by Editor/Lua
		std::string id = "NewSlot";

		// Link to SoundAsset via GUID
		xresource::full_guid audio_guid{};

		// Basic playback controls
		float volume		= 1.0f;
		float pitch			= 1.0f;
		bool loop			= false;
		bool useFade		= false;
		float fadeDuration	= 1.0f;

		// Default to SFX
		AudioCategory category = AudioCategory::SFX;

		// Since its default SFX, its a general ambience
		AmbienceType ambienceType = AmbienceType::General;

		// 2D / 3D Behaviour
		SpatialMode spatial = SpatialMode::ThreeD;

		// Only when spatial == ThreeD
		float minDistance = 1.0f;
		float maxDistance = 25.0f;

		// Auto-play when entity becomes active
		bool playOnStart = false;
	};

	// Keep at least one slot for editor UX
	std::vector<Slot> Slots;

	AudioComponent()
		: Slots(1) {}

	std::size_t GetSlotCount() const { return Slots.size(); }

	Slot* GetSlot(std::size_t index)
	{
		if (index >= Slots.size())
			return nullptr;
		return &Slots[index];
	}

	Slot& PrimarySlot()
	{
		if (Slots.empty())
			Slots.emplace_back();
		return Slots.front();
	}

	const Slot& PrimarySlot() const
	{
		if (Slots.empty())
		{
			static Slot defaultSlot{};
			return defaultSlot;
		}
		return Slots.front();
	}

	Slot& AddSlot(const std::string& suggestedId = "NewSlot")
	{
		Slots.emplace_back();
		Slots.back().id = suggestedId;
		return Slots.back();
	}

	bool RemoveSlot(std::size_t index)
	{
		if (Slots.size() <= 1 || index >= Slots.size())
			return false;

		Slots.erase(Slots.begin() + static_cast<std::vector<Slot>::difference_type>(index));
		return true;
	}

	// Find a slot by its ID
	// Return nullptr if not found
	Slot* FindSlot(const std::string& slotId)
	{
		for (auto& s : Slots)
		{
			if (s.id == slotId)
				return &s;
		}

		return nullptr;
	}

	const Slot* FindSlot(const std::string& slotId) const
	{
		for (const auto& s : Slots)
		{
			if (s.id == slotId)
				return &s;
		}

		return nullptr;
	}

	bool HasSlot(const std::string& slotId) const
	{
		return FindSlot(slotId) != nullptr;
	}

};