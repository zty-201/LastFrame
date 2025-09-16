#ifndef GRAPHICS_SYSTEM_HPP
#define GRAPHICS_SYSTEM_HPP

#include "ISystem.hpp"
#include "Scene.h"

#define ANINSTANCE(x, z)\
         { MATERIAL_OBJECT, glm::translate(glm::mat4(1.0f), {x, 0.0f, z}) }

#define INSTANCES(x)\
                   ANINSTANCE(x, -20.0f-5.0f),\
                   ANINSTANCE(x, -18.0f-5.0f),\
                   ANINSTANCE(x, -16.0f-5.0f),\
                   ANINSTANCE(x, -14.0f-5.0f),\
                   ANINSTANCE(x, -12.0f-5.0f),\
                   ANINSTANCE(x, -10.0f-5.0f),\
                   ANINSTANCE(x,  -8.0f-5.0f),\
                   ANINSTANCE(x,  -6.0f-5.0f),\
                   ANINSTANCE(x,  -4.0f-5.0f),\
                   ANINSTANCE(x,  -2.0f-5.0f),\
                   ANINSTANCE(x,   0.0f-5.0f),\
                   ANINSTANCE(x,   2.0f-5.0f),\
                   ANINSTANCE(x,   4.0f-5.0f),\
                   ANINSTANCE(x,   6.0f-5.0f),\
                   ANINSTANCE(x,   8.0f-5.0f),\
                   ANINSTANCE(x,  10.0f-5.0f),\
                   ANINSTANCE(x,  12.0f-5.0f),\
                   ANINSTANCE(x,  14.0f-5.0f),\
                   ANINSTANCE(x,  16.0f-5.0f),\
                   ANINSTANCE(x,  18.0f-5.0f),\
                   ANINSTANCE(x,  20.0f-5.0f)

namespace EngineFramework
{
    class GraphicsSystem : public ISystem
    {
    public:
        GraphicsSystem();
        ~GraphicsSystem();

        // ISystem implementation
        void Initialize() override;
        void Update(float deltaTime) override;
        void PostUpdate(float deltaTime) override;

		// Accessor for the scene
        GLFWwindow* GetGLFWwindow() { return window; }
    private:

        std::unique_ptr<Scene> scene;
        
		GLFWwindow* window = nullptr;

        glm::vec2 screen = { WIDTH,HEIGHT };
        glm::vec3 fogColor; // Example: gray fog color
        float fogMinDist;
        float fogMaxDist;
        int shadingModel;
        // Setup methods
        void SetupGLFW();
        void SetupImGui();
        void SetupGraphics();
        
        // Callback handling
        static void ErrorCallback(int error, const char* msg);
    };
}

#endif // GRAPHICS_SYSTEM_HPP