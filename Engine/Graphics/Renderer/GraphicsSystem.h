#ifndef GRAPHICS_SYSTEM_HPP
#define GRAPHICS_SYSTEM_HPP

#include "ISystem.hpp"
#include "scene.h"

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
        
        // Graphics-specific methods
        GLFWwindow* GetWindow() const { return scene.window; }
        bool ShouldClose() const { return glfwWindowShouldClose(scene.window); }
        
    private:
        Scene scene;
        
        // Setup methods
        void SetupGLFW();
        void SetupImGui();
        void SetupGraphics();
        
        // Callback handling
        static void ErrorCallback(int error, const char* msg);
    };
}

#endif // GRAPHICS_SYSTEM_HPP