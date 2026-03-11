#ifndef UICOMPONENT_H
#define UICOMPONENT_H

/*

    I NEED TO CHANGE THIS ABIT TO MAKE LIKE THIS MORE USEFUL
    ENABLE LOGIC AND EVENT SYSTEM TO ALLOW VISIBILITY FOR OVERLAY UI COMPONENT

    - MANAGE BY THE UI_SYSTEM for overlay menus 
    - For game elements like those pop up icons, managed by scripts
*/

struct UIComponent
{
    xresource::full_guid textureid; // Name of the file to load
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

    //================ extra stuff for overlays =====================
    bool belong_to_menu;
    std::string Overlay_name =""; //see which overlay it belongs to
    std::string event_received = ""; //event to trigger visibility
    bool hide_first;
};

#endif // UICOMPONENT_H