#pragma once
#ifndef AUDIO_SYSTEM_HPP
#define AUDIO_SYSTEM_HPP

#include <string>
#include <unordered_map>
#include <filesystem>
#include "ISystem.hpp"

/*
* ----- Features of an Audio System -----
*   1. Initialize & Shutdown FMOD
*   2. Load audio file by name and file path
*   3. Play oneshot or loop an audio
*   4. Stop or Pause an audio
*   5. Set master volume, audio type volume (eg. SFX, Music, Ambient)
*/

namespace FMOD {
    class System;
    class Sound;
    class ChannelGroup;
    class Channel;
}

namespace EngineFramework {

    

} // namespace EngineFramework
#endif // AUDIO_SYSTEM_HPP
