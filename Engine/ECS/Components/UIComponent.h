#ifndef UICOMPONENT_H
#define UICOMPONENT_H

struct UIComponent
{
    xresource::full_guid textureid; // Name of the file to load
    glm::vec2 position;      // X, Y in Pixels (from Bottom-Left or Top-Left)
    glm::vec2 size;          // Width, Height in Pixels
    glm::vec3 color = glm::vec3(1.0f); // Default White (No tint)
    bool visible = true;
    int layer = 0;           // Optional: Render order (0=Back, 10=Front)
};

#endif // UICOMPONENT_H