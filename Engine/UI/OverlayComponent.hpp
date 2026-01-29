
#ifndef OVERLAY_COMPONENT_HPP
#define OVERLAY_COMPONENT_HPP

#include<string>
#include<cstdint>
#include<cstddef>

//enable for window debug first (avoid android cause android now got issue)
#if defined(DEBUG) && defined(PLATFORM_WINDOWS)

struct OverlayComponent {

    std::string idle_texture; //default
    std::string belongToMenu_name; //most probably should not be editable, as it go alongs with the file being opened
    bool is_enable; //tells the graphics when it is enabled.

};

#endif
#endif




