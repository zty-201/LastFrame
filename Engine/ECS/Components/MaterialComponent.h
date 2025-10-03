#ifndef MATERIAL_COMPONENT
#define MATERIAL_COMPONENT
struct MaterialComponent
{
    float rough;
    float metal;
    glm::vec3 color;
    glm::vec3 Ka;
    glm::vec3 Kd;
    glm::vec3 Ks;
    float shininess;
    float effect; 
	std::string materialName;
};

// Define some default materials using the new structure (to be deleted later)
const MaterialComponent MATERIAL_PBR_PHONG({ 0.8f, 0.2f, {1.0f, 0.8f, 0.2f}, {0.1f, 0.1f, 0.1f}, {0.7f, 0.7f, 0.7f}, {1.0f, 1.0f, 1.0f}, 128.0f, 0.0f, "PBR"});
const MaterialComponent MATERIAL_DISCARD({ 0.5f, 0.0f, {0.2f, 0.8f, 0.2f}, {0.1f, 0.1f, 0.1f}, {0.7f, 0.7f, 0.7f}, {1.0f, 1.0f, 1.0f}, 32.0f, 1.0f, "Discard"});
const MaterialComponent MATERIAL_CARTOON({ 0.5f, 0.0f, {0.8f, 0.2f, 0.8f}, {0.1f, 0.1f, 0.1f}, {0.7f, 0.7f, 0.7f}, {1.0f, 1.0f, 1.0f}, 32.0f, 2.0f, "Cartoon"});
const MaterialComponent MATERIAL_CHECKERBOARD({ 0.9f, 0.0f, {1.0f, 1.0f, 1.0f}, {0.1f, 0.1f, 0.1f}, {0.7f, 0.7f, 0.7f}, {1.0f, 1.0f, 1.0f}, 32.0f, 3.0f, "Checkerboard"});

// --- Other Scene Entity Components ---
#endif
