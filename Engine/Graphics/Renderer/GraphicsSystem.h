/**
 * @file GraphicsSystem.h
 * @brief Declaration of the GraphicsSystem class for managing rendering operations.
 *
 * This file defines the `GraphicsSystem` class, which is responsible for initializing
 * and managing the graphics pipeline in the EngineFramework. It handles window creation,
 * OpenGL context setup, rendering scenes, and managing framebuffers, shaders, and other
 * graphics-related resources.
 *
 * Key functionalities:
 * - Initialization of GLFW and OpenGL contexts.
 * - Management of rendering passes, including scene rendering and skybox rendering.
 * - Handling of events such as window resizing and toggling the level editor.
 * - Integration with the asset manager for loading and caching meshes, textures, and shaders.
 *
 * @author Zhang TongYan
 * @date October, 2025
 */

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

        void RenderScene(); 
        void LoadAndCacheMesh(const std::string& meshName);
        void ApplyPendingResize();

        static GraphicsSystem& GetInstance()
        {
            return *this_graphics_ptr;
        }

        // Mutator for boolean
        void SetDebugStatus(bool enable) { show_debugDraw = enable; }
        bool GetDebugStatus() { return show_debugDraw; }

		// Accessor for the scene
#ifdef PLATFORM_WINDOWS
        GLFWwindow* GetGLFWwindow() { return pWindow; }
#endif
		std::unique_ptr<Scene>& GetScene() { return pScene; }
		GLuint GetSceneTextureID() const { return sceneTextureID; }
        std::pair<int, int> GetWindowSize() const { return std::pair<int, int>(static_cast<int>(screen.x), static_cast<int>(screen.y)); }

    private:

#ifdef PLATFORM_WINDOWS
        GLFWwindow* pWindow = nullptr;
#endif
        // A map to cache loaded mesh data (VAO, index count)
        xresource::full_guid geometric_shader_guid;
        xresource::full_guid lighting_shader_guid;
		xresource::full_guid forward_shader_guid;

        // For fullscreen quad in lighting pass
        GLuint fullscreenQuadVAO = 0;
        GLuint fullscreenQuadVBO = 0;

        std::unordered_map<std::string, std::pair<GLuint, size_t>> meshCache;
        std::unordered_map<std::string, std::pair<GLuint, size_t>> meshCacheWire;

        std::unique_ptr<Scene> pScene;

        FBO mainFBO;
        FBO gBufferFBO;

        GLuint sceneTextureID;

        glm::vec2 screen = { WIDTH,HEIGHT };
        
        bool show_debugDraw = false;

        bool m_hasPendingResize = false;
        bool m_isRenderingToFBO = false;
        int m_pendingWidth = 0;
        int m_pendingHeight = 0;

        // Setup methods
        void SetupGLFW();
        void SetupGraphics();
		void GeometryPass(glm::mat4& V, glm::mat4& P);
		void LightingPass(glm::mat4& V);
		void ForwardRenderPass(glm::mat4& V, glm::mat4& P, GLuint targetFBO);
        void InitializeFullscreenQuad();
		void CalculateModelMatrix(const TransformComponent& transform, glm::mat4& modelMatrix);

        //==Event Handlers, one for each type of event.

        //When level editor is toggled on/off.
        void HANDLE_EVENT_LevelEditorToggled(Event_Level_Editor_Toggled&);
        //When drawing area should be resized.
        void HANDLE_EVENT_WindowResized(Event_Window_Resized&);

        static inline GraphicsSystem* this_graphics_ptr = nullptr;
    };
}

#endif // GRAPHICS_SYSTEM_HPP