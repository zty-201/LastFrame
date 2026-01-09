#ifndef AUDIO_COMPONENT_SERIALIZATION_HPP
#define AUDIO_COMPONENT_SERIALIZATION_HPP

#include "AudioComponent.h"

#include <nlohmann/json.hpp>

inline void NormalizeAudioComponentJson(nlohmann::json& componentJson)
{
	if (!componentJson.is_object())
		return;

	if (componentJson.contains("Clips"))
		return;

	const bool hasLegacyFields =
		componentJson.contains("audio_guid") ||
		componentJson.contains("volume") ||
		componentJson.contains("loop") ||
		componentJson.contains("playOnStart") ||
		componentJson.contains("is3D") ||
		componentJson.contains("minDistance") ||
		componentJson.contains("maxDistance");

	if (!hasLegacyFields)
		return;

	nlohmann::json clipJson = nlohmann::json::object();

	if (componentJson.contains("audio_guid"))
	{
		clipJson["audio_guid"] = componentJson["audio_guid"];
	}

	const AudioComponent::AudioClip defaults{};

	clipJson["volume"] = componentJson.value("volume", defaults.volume);
	clipJson["loop"] = componentJson.value("loop", defaults.loop);
	clipJson["playOnStart"] = componentJson.value("playOnStart", defaults.playOnStart);
	clipJson["is3D"] = componentJson.value("is3D", defaults.is3D);
	clipJson["minDistance"] = componentJson.value("minDistance", defaults.minDistance);
	clipJson["maxDistance"] = componentJson.value("maxDistance", defaults.maxDistance);

	componentJson["Clips"] = nlohmann::json::array();
	componentJson["Clips"].push_back(std::move(clipJson));
}

#endif // AUDIO_COMPONENT_SERIALIZATION_HPP
