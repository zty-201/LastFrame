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


        static GraphicsSystem& GetInstance()
        {
            return *this_graphics_ptr;
        }

		// Accessor for the scene
#ifdef PLATFORM_WINDOWS
        GLFWwindow* GetGLFWwindow() { return pWindow; }
#endif
		std::unique_ptr<Scene>& GetScene() { return pScene; }
		bool IsAltDown() const { return alt_down; }
		bool IsLButtonDown() const { return lbutton_down; }
		bool IsRButtonDown() const { return rbutton_down; }
		bool IsShiftDown() const { return mode_shift; }

        
    private:

        std::unique_ptr<Scene> pScene;
		static inline GraphicsSystem* this_graphics_ptr = nullptr;
#ifdef PLATFORM_WINDOWS
		GLFWwindow* pWindow = nullptr;
#endif
        glm::vec2 screen = { WIDTH,HEIGHT };
        glm::vec3 fogColor; // Example: gray fog color
        float fogMinDist;
        float fogMaxDist;
        int shadingModel;
        // Setup methods
        void SetupWindow();
        void SetupImGui();
        void SetupGraphics();

        bool lbutton_down = false;
        bool rbutton_down = false;
        bool mode_shift = false;
        bool alt_down = false;

    };
}

#endif // GRAPHICS_SYSTEM_HPP