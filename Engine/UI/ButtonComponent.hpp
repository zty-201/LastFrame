
#ifndef BUTTON_COMPONENT
#define BUTTON_COMPONENT

#include<string>
#include<cstdint>
#include<cstddef>

//enable for window debug first (avoid android cause android now got issue)
#if defined(DEBUG) && defined(PLATFORM_WINDOWS)


enum BUTTON_STATE {
    IDLE, HOVER, CLICKED, RELEASED
};

struct ButtonComponent {

    BUTTON_STATE button_state;

    std::string idle_texture;
    std::string hover_texture;
    std::string clicked_texture;
    std::string event; //name of the event, maybe theres a better way insteaad of string?

    bool is_hover;
    bool is_clicked;
    bool is_released; //trigger even when it is released
    bool is_active; //value to let dev know if its functioning (for debug purposes)


#if defined(DEBUG) && defined(PLATFORM_WINDOWS)
    /*
    * pause the clickability when simulation is not running
    * only in editor (debug)
    */
    bool is_clickable;
#endif


};

#endif


#endif

