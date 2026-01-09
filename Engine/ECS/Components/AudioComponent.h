#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include "xresource_guid.h"


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