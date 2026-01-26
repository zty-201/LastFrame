#ifndef AUDIO_COMPONENT_SERIALIZATION_HPP
#define AUDIO_COMPONENT_SERIALIZATION_HPP

#include "AudioComponent.h"

#include <nlohmann/json.hpp>

inline void NormalizeAudioComponentJson(nlohmann::json& componentJson)
{
    using json = nlohmann::json;

	if (!componentJson.is_object())
		return;

	if (componentJson.contains("Slots"))
		return;

    auto clip_to_slot = [](const json& clipJson, std::size_t index) -> json
        {
            json slotJson = json::object();

            // id: legacy didn't have one; generate stable-ish default
            slotJson["id"] = clipJson.value("id", std::string("Slot") + std::to_string(index + 1));

            if (clipJson.contains("audio_guid") && !clipJson["audio_guid"].is_null())
                slotJson["audio_guid"] = clipJson["audio_guid"];

            // Defaults (match your new component defaults)
            slotJson["volume"] = clipJson.value("volume", 1.0f);
            slotJson["pitch"] = clipJson.value("pitch", 1.0f);
            slotJson["loop"] = clipJson.value("loop", false);
            slotJson["playOnStart"] = clipJson.value("playOnStart", false);

            // Spatial: legacy used is3D bool
            // New format stores spatial enum as int (0=TwoD, 1=ThreeD) in our recommended serializer.
            if (clipJson.contains("spatial"))
            {
                slotJson["spatial"] = clipJson["spatial"]; // already normalized elsewhere
            }
            else
            {
                const bool is3D = clipJson.value("is3D", true);
                slotJson["spatial"] = is3D ? static_cast<int>(AudioComponent::SpatialMode::ThreeD)
                    : static_cast<int>(AudioComponent::SpatialMode::TwoD);
            }

            slotJson["minDistance"] = clipJson.value("minDistance", 1.0f);
            slotJson["maxDistance"] = clipJson.value("maxDistance", 25.0f);

            // Category: legacy had no category. Default to SFX.
            // Stored as int (matches recommended serializer).
            if (clipJson.contains("category"))
                slotJson["category"] = clipJson["category"]; // already present
            else
                slotJson["category"] = static_cast<int>(AudioComponent::AudioCategory::SFX);

            return slotJson;
        };

    // Case 2: Legacy Clips[] format
    if (componentJson.contains("Clips") && componentJson["Clips"].is_array())
    {
        const json& clips = componentJson["Clips"];
        json slots = json::array();

        std::size_t idx = 0;
        for (const auto& c : clips)
        {
            if (!c.is_object())
                continue;

            slots.push_back(clip_to_slot(c, idx));
            ++idx;
        }

        if (!slots.empty())
            componentJson["Slots"] = std::move(slots);

        // Optionally keep legacy field for backwards compatibility; otherwise remove.
        componentJson.erase("Clips");
        return;
    }

    // Case 3: Very old flat format (fields live on the component object)
    const bool hasLegacyFields =
        componentJson.contains("audio_guid") ||
        componentJson.contains("volume") ||
        componentJson.contains("pitch") ||
        componentJson.contains("loop") ||
        componentJson.contains("playOnStart") ||
        componentJson.contains("is3D") ||
        componentJson.contains("minDistance") ||
        componentJson.contains("maxDistance") ||
        componentJson.contains("category") ||
        componentJson.contains("spatial");

    if (!hasLegacyFields)
        return;

    json legacyClip = json::object();

    // Move/copy recognized fields into a clip-like object
    auto move_if_present = [&](const char* key)
        {
            if (componentJson.contains(key))
                legacyClip[key] = componentJson[key];
        };

    move_if_present("audio_guid");
    move_if_present("volume");
    move_if_present("pitch");
    move_if_present("loop");
    move_if_present("playOnStart");
    move_if_present("is3D");
    move_if_present("minDistance");
    move_if_present("maxDistance");
    move_if_present("category");
    move_if_present("spatial");
    move_if_present("id");

    componentJson["Slots"] = json::array();
    componentJson["Slots"].push_back(clip_to_slot(legacyClip, 0));

    // Clean up old flat keys so we don't store duplicate/conflicting data.
    componentJson.erase("audio_guid");
    componentJson.erase("volume");
    componentJson.erase("pitch");
    componentJson.erase("loop");
    componentJson.erase("playOnStart");
    componentJson.erase("is3D");
    componentJson.erase("minDistance");
    componentJson.erase("maxDistance");
    componentJson.erase("category");
    componentJson.erase("spatial");
    componentJson.erase("id");
}

#endif // AUDIO_COMPONENT_SERIALIZATION_HPP
