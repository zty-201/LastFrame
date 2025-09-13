#ifndef MATERIAL_H
#define MATERIAL_H

#include "ShaderProgram.h"

    // Predefined material definitions using macros for common materials (Phong and PBR)

    // Blue Phong material with shininess and color properties
#define PHONG_MATERIAL_BLUE\
        {\
            { "material.shininess", { 100.0f } },\
            { "material.Ka", { 0.3f, 0.5f, 0.9f } },\
            { "material.Kd", { 0.3f, 0.5f, 0.9f } },\
            { "material.Ks", { 0.8f, 0.8f, 0.8f } },\
            { "material.effect", { 0.0f } }\
        }

    // Orange Phong material with shininess and color properties
#define PHONG_MATERIAL_ORANGE\
        {\
            { "material.shininess", { 100.0f } },\
            { "material.Ka", { 0.9f, 0.5f, 0.3f } },\
            { "material.Kd", { 0.9f, 0.5f, 0.3f } },\
            { "material.Ks", { 0.8f, 0.8f, 0.8f } },\
            { "material.effect", { 0.0f } }\
        }

    // White Phong material with shininess and color properties
#define PHONG_MATERIAL_WHITE\
        {\
            { "material.shininess", { 100.0f } },\
            { "material.Ka", { 1.0f, 1.0f, 1.0f } },\
            { "material.Kd", { 1.0f, 1.0f, 1.0f } },\
            { "material.Ks", { 1.0f, 1.0f, 1.0f } },\
            { "material.effect", { 0.0f } }\
        }

    // Reflective and refractive material with color and reflection factor
#define MATERIAL_REFLECT_REFRACT\
        {\
            { "material.color", { 0.5f, 0.5f, 0.5f } },\
            { "material.reflectionFactor", { 0.9f } },\
            { "material.eta", { 0.94f } },\
            { "material.effect", { 0.0f } }\
        }

    // PBR (Physically-Based Rendering) material properties for different colors
#define MATERIAL_PROPERTIES_PBR_RED\
            { "material.rough", { 0.3f } },\
            { "material.metal", { 0.0f } },\
            { "material.color", { 1.0f, 0.0f, 0.0f } }

#define MATERIAL_PROPERTIES_PBR_GREEN\
            { "material.rough", { 0.3f } },\
            { "material.metal", { 0.0f } },\
            { "material.color", { 0.0f, 1.0f, 0.0f } }

#define MATERIAL_PROPERTIES_PBR_BLUE\
            { "material.rough", { 0.3f } },\
            { "material.metal", { 0.0f } },\
            { "material.color", { 0.0f, 0.0f, 1.0f } }

#define MATERIAL_PROPERTIES_PBR_GREY\
            { "material.rough", { 0.3f } },\
            { "material.metal", { 0.0f } },\
            { "material.color", { 0.3f, 0.3f, 0.3f } }

    // Phong material properties for different colors
#define MATERIAL_PROPERTIES_PHONG_RED\
            { "material.shininess", { 100.0f } },\
            { "material.Ka", { 0.0f, 0.0f, 0.0f } },\
            { "material.Kd", { 1.0f, 0.0f, 0.0f } },\
            { "material.Ks", { 1.0f, 0.0f, 0.0f } }   

#define MATERIAL_PROPERTIES_PHONG_GREEN\
            { "material.shininess", { 100.0f } },\
            { "material.Ka", { 0.0f, 0.0f, 0.0f } },\
            { "material.Kd", { 0.0f, 1.0f, 0.0f } },\
            { "material.Ks", { 0.0f, 1.0f, 0.0f } } 

#define MATERIAL_PROPERTIES_PHONG_BLUE\
            { "material.shininess", { 100.0f } },\
            { "material.Ka", { 0.0f, 0.0f, 0.0f } },\
            { "material.Kd", { 0.0f, 0.0f, 1.0f } },\
            { "material.Ks", { 0.0f, 0.0f, 1.0f } } 

#define MATERIAL_PROPERTIES_PHONG_GREY\
            { "material.shininess", { 100.0f } },\
            { "material.Ka", { 0.0f, 0.0f, 0.0f } },\
            { "material.Kd", { 0.3f, 0.3f, 0.3f } },\
            { "material.Ks", { 0.3f, 0.3f, 0.3f } } 

    // Final predefined materials, combining PBR and Phong with some effects
#define MATERIAL_PBR\
            {\
                MATERIAL_PROPERTIES_PBR_RED,\
                { "material.effect", { 0.0f } }\
            }

#define MATERIAL_PBR_PHONG\
            {\
                MATERIAL_PROPERTIES_PBR_RED,\
                MATERIAL_PROPERTIES_PHONG_RED,\
                { "material.effect", { 0.0f } }\
            }

#define MATERIAL_DISCARD  \
            {\
                MATERIAL_PROPERTIES_PBR_GREEN,\
                MATERIAL_PROPERTIES_PHONG_GREEN,\
                { "material.effect", { 1.0f } }\
            }

#define MATERIAL_CARTOON  \
            {\
                MATERIAL_PROPERTIES_PBR_BLUE,\
                MATERIAL_PROPERTIES_PHONG_BLUE,\
                { "material.effect", { 2.0f } }\
            }

#define MATERIAL_CHECKERBOARD  \
            {\
                MATERIAL_PROPERTIES_PBR_GREY,\
                MATERIAL_PROPERTIES_PHONG_GREY,\
                { "material.effect", { 3.0f } }\
            }

    // Material for a plane using the blue Phong material
#define MATERIAL_PLANE  PHONG_MATERIAL_BLUE

// Material for an object using the orange Phong material
#define MATERIAL_OBJECT PHONG_MATERIAL_ORANGE

//
// Material base class for defining materials with parameters for rendering. 
// This class and the macros simplify the creation and usage of materials 
// in a graphics framework, allowing for easy management of material
// properties and their application in shaders.
//
struct Material
{
    // A map of parameter names (strings) to values (vectors of floats)
    std::map<std::string, std::vector<float>> params;

    // Constructor initializes the material parameters using an initializer list
    Material(const std::initializer_list< std::pair<std::string, std::initializer_list<float>> >& params)
        : params{ }
    {
        // Copy parameters into the internal map
        for (const std::pair < std::string, std::initializer_list<float>>& p : params)
            this->params.insert(p);
    }

    // Set the material uniforms in a shader program
    void setUniforms(ShaderProgram* pProgram) const
    {
        pProgram->use(); // Activate the shader program
        // Loop over each parameter and set it in the shader program
        for (const std::pair <std::string, std::vector<float>>& p : params)
            pProgram->setUniform(p.first, p.second); // Set uniform by name and value
        pProgram->disuse(); // Deactivate the shader program
    }
};

#endif // !MATERIAL_H
