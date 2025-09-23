#ifndef GRAPHICS_SYSTEM_HPP
#define GRAPHICS_SYSTEM_HPP

#include "ISystem.hpp"
#include "Scene.h"

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
		GLuint GetSceneTextureID() const { return sceneTextureID; }
        std::pair<int, int> GetWindowSize() const { return std::pair<int, int>(static_cast<int>(screen.x), static_cast<int>(screen.y)); }

    private:
        std::unique_ptr<Scene> pScene;
#ifdef PLATFORM_WINDOWS
		GLFWwindow* pWindow = nullptr;
#endif

        //Scene Framebuffer and Texture.
        FBO scene_framebuffer;
        GLuint sceneTextureID;

        glm::vec2 screen = { WIDTH,HEIGHT };
        glm::vec3 fogColor; // Example: gray fog color
        float fogMinDist;
        float fogMaxDist;
        int shadingModel;
        static inline GraphicsSystem* this_graphics_ptr = nullptr;

        // Setup methods
        void SetupGLFW();
        void SetupGraphics();

        //Toggle drawing to framebuffer.
        void DrawSceneToFramebuffer(bool);

        //==Event Handlers, one for each type of event.

        //When level editor is toggled on/off.
        void HANDLE_EVENT_LevelEditorToggled(Event_Level_Editor_Toggled&);
        //When drawing area should be resized.
        void HANDLE_EVENT_WindowResized(Event_Window_Resized&);

    };
}

#endif // GRAPHICS_SYSTEM_HPP