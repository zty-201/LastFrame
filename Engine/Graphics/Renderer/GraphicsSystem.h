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
#include "scene.h"
#include "InstancingSystem.h"

struct VolumetricUBO
{
    glm::mat4 view;
    glm::mat4 projection;
    glm::mat4 view_proj;
    glm::mat4 prev_view_proj;
    glm::mat4 light_view_proj;
    glm::mat4 inv_view_proj;
    glm::vec4 light_direction;
    glm::vec4 light_color;
    glm::vec4 camera_position;
    glm::vec4 bias_near_far_pow; // x: bias, y: near, z: far, w: power
    glm::vec4 aniso_density_scattering_absorption;
    glm::vec4 time; // x: current time
    glm::ivec4 width_height;
};

namespace EngineFramework
{
    namespace TextureUnits
    {
        // Define texture units for the new shader bindings
        const int MESH_ALBEDO_UNIT = 0;
        const int MESH_NORMAL_UNIT = 1;
        const int MESH_METALLIC_UNIT = 2;
        const int MESH_ROUGHNESS_UNIT = 3;
        const int SHADOW_MAP_UNIT = 4;
        const int VOLUMETRIC_VOXEL_GRID_UNIT = 5;
        const int SKYBOX_CUBEMAP_UNIT = 6;
        const int VOLUMETRIC_BLUE_NOISE_UNIT = 7;
        const int VOLUMETRIC_HISTORY_UNIT = 8;
    }

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

        static GraphicsSystem& GetInstance(){ return *this_graphics_ptr; }

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

        //==Camera Manipulation

        //Returns the active camera, or sets a random camera as active if none are. Returns entt::null if no camera found.
        entt::entity GetFirstActiveCamera()
        {
            auto view = registry.view<CameraComponent>();
            if (view.begin() == view.end()) //No camera component in scene
            {
                return entt::null;
            }
            for (entt::entity camera : view)
            {
                if(registry.get<CameraComponent>(camera).is_active)
                {
                    return camera;
                }
            }
            //None of the cameras are active, set the first one as active.
            registry.get<CameraComponent>(view.front()).is_active = true;
            return view.front();
        }

        void CalculateModelMatrix(const TransformComponent& transform, glm::mat4& modelMatrix);

        // Simple get and set functions for Fog Control Parameters
        // Anisotrophy
        void SetAnisotrophy(float value) { m_anisotropy = value; }
        float GetAnisotrophy() const { return m_anisotropy; }

		// Density
        void SetDensity(float value) { m_density = value; }
        float GetDensity() const { return m_density; }


        // Scattering
        void SetScattering(float value) { m_scattering = value; }
        float GetScattering() const { return m_scattering; }

		// Absorption
        void SetAbsorption(float value) { m_absorption = value; }
        float GetAbsorption() const { return m_absorption; }

		// Shadow Bias
        void SetShadowBias(float value) { m_shadow_bias = value; }
        float GetShadowBias() const { return m_shadow_bias; }

		// Depth Power
        void SetDepthPower(float value) { m_depth_power = value; }
        float GetDepthPower() const { return m_depth_power; }

		// Ambient Light Intensity
        void SetAmbientLightIntensity(float value) { m_ambient_light_intensity = value; }
        float GetAmbientLightIntensity() const { return m_ambient_light_intensity; }

		// Temporal Accumulation
        bool GetTemporalAccumulation() const { return m_temporal_accumulation; }
        void SetTemporalAccumulation(bool value) { m_temporal_accumulation = value; }

		// Tricubic Filtering
        bool GetTricubicFiltering() const { return m_tricubic_filtering; }
        void SetTricubicFiltering(bool value) { m_tricubic_filtering = value; }

    private:

#ifdef PLATFORM_WINDOWS
        GLFWwindow* pWindow = nullptr;
#endif

        //std::unique_ptr<InstancingSystem> m_instancingSystem;
        // A map to cache loaded mesh data (VAO, index count)
        xresource::full_guid mesh_shader_guid;
        xresource::full_guid skybox_shader_guid;
        xresource::full_guid shadow_shader_guid;
        xresource::full_guid light_injection_shader_guid;
        xresource::full_guid ray_march_shader_guid;

        GLuint m_volumetricUBO = 0;

        std::unordered_map<std::string, std::shared_ptr<Mesh::MeshData>> meshCache;
        std::unordered_map<std::string, std::pair<GLuint, size_t>> meshCacheWire;

        std::unique_ptr<Scene> pScene;

        FBO mainFBO;
        FBO m_shadowFBO;

        GLuint sceneTextureID;
        GLuint m_shadowMapTextureID;
        
        // 3D textures for volumetric fog passes
        GLuint m_ray_march_voxel_grid = 0;
        GLuint m_temporal_integration_voxel_grid[2] = { 0, 0 };

        GLuint m_defaultWhiteTexture = 0;
        GLuint m_defaultNormalTexture = 0;
        std::vector<GLuint> m_blue_noise_textures;
        
        // Fog control parameters
        float m_anisotropy = 0.6f;      // Slightly forward scattering
        float m_density = 1.0f;         // Much lower density (was 0.15)
        float m_scattering = 0.0f;
        float m_absorption = 0.0f; 
        float m_shadow_bias = 0.002f;
        float m_depth_power = 1.0f; // For exponential depth mapping
        float m_ambient_light_intensity = 0.001f;
        bool m_temporal_accumulation = true;
        bool m_tricubic_filtering = false;
        int m_frame_idx = 0;
        bool m_ping_pong = false;
        glm::mat4 m_prev_view_projection;
        glm::mat4 m_lightSpaceMatrix;

        glm::vec2 screen = { WIDTH,HEIGHT };
        
        bool show_debugDraw = false;
        bool m_isRenderingToFBO = false;

        // Setup methods
        void SetupGLFW();
        void SetupGraphics();

		// Rendering passes
        void ShadowPass();
        void MeshPass();
        void SkyboxPass();
        void VolumetricLightInjectionPass();
        void VolumetricRayMarchPass();
        void UpdateUBOs(const CameraComponent& camera, const glm::mat4& V, const glm::mat4& P);
        void PreGeneratePrimitives();
        void LoadBlueNoiseTextures();

        //==Event Handlers, one for each type of event.
        void HANDLE_EVENT_LevelEditorToggled(Event_Level_Editor_Toggled&);
        void HANDLE_EVENT_WindowResized(Event_Window_Resized&);

        static inline GraphicsSystem* this_graphics_ptr = nullptr;
    };
}

#endif // GRAPHICS_SYSTEM_HPP