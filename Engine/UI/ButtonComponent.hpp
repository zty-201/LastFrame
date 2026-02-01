
#ifndef BUTTON_COMPONENT
#define BUTTON_COMPONENT

#include<string>
#include<cstdint>
#include<cstddef>

//enable for window debug first (avoid android cause android now got issue)
#if defined(DEBUG) && defined(PLATFORM_WINDOWS)


enum BUTTON_STATE {
    NO_STATE, IDLE, HOVER, CLICKED, RELEASED, TRIGGERED
};

struct ButtonComponent {

    BUTTON_STATE curr_button_state;
    BUTTON_STATE prev_button_state;

    xresource::full_guid idle_texture;
    xresource::full_guid hover_texture;
    xresource::full_guid clicked_texture;
    std::string event_trigger;
    std::string event_hover;
    

    bool is_hover;
    bool is_clicked;
    bool is_released; //trigger even when it is released
    bool is_active; //value to let dev know if its functioning (for debug purposes)


#if defined(DEBUG) && defined(PLATFORM_WINDOWS)
    /*
    * pause the clickability when simulation is not running
    * only in editor (debug)
    */
    bool is_functioning;
#endif


};

#endif


#endif

