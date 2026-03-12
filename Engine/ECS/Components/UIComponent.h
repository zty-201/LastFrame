#ifndef UICOMPONENT_H
#define UICOMPONENT_H

/*

    I NEED TO CHANGE THIS ABIT TO MAKE LIKE THIS MORE USEFUL
    ENABLE LOGIC AND EVENT SYSTEM TO ALLOW VISIBILITY FOR OVERLAY UI COMPONENT

    - MANAGE BY THE UI_SYSTEM for overlay menus 
    - For game elements like those pop up icons, managed by scripts
*/

#include <cstdint>

namespace EngineFramework
{
    enum class AudioGroup : uint8_t;
}

struct UIComponent
{
    xresource::full_guid textureid; // Name of the file to load
    xresource::full_guid videoid;   // Optional video asset guid
    bool play_video_audio = true;
    float video_audio_volume = 1.0f;
    int video_audio_group = 1; // matches EngineFramework::AudioGroup enum
    glm::vec2 position;      // X, Y in Pixels (from Bottom-Left or Top-Left)
    glm::vec2 size;          // Width, Height in Pixels
    glm::vec3 color = glm::vec3(1.0f); // Default White (No tint)
    bool visible = true;
	float luminance = 1.0f; // Brightness multiplier
    bool lock_luminance = false; // NEW: prevents fade system from modifying luminance
    int layer = 0;           // Optional: Render order (0=Back, 10=Front)
    bool is_NDC = false;
    glm::vec2 position_NDC;
    glm::vec2 size_NDC;
    bool use_subtitle_reveal = false;
    float subtitle_reveal_progress = 1.0f; // 0 = fully hidden, 1 = fully shown
    float subtitle_reveal_softness = 0.03f; // soft edge width

    //================ extra stuff for overlays =====================
    bool belong_to_menu;
    std::string Overlay_name =""; //see which overlay it belongs to
    std::string event_received = ""; //event to trigger visibility
    bool hide_first;
};

#endif // UICOMPONENT_H
