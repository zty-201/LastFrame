#ifndef LIGHT_COMPONENT
#define LIGHT_COMPONENT
//using LightComponent = Light;
struct LightComponent
{
    glm::vec3 position{};       // The position of the light in 3D space
    std::vector<glm::vec3> L{}; // Intensity values for ambient, diffuse, and specular components
};

#endif