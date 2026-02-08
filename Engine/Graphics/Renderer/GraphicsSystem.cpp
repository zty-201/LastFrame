#include "pch.hpp"
#include "global.hpp"
#include "Debug.h"
#include "GraphicsSystem.h"
#include "InputManager.hpp"
#include "AssetManager.hpp"
#include "MaterialComponent.h"
#include "AnimationComponent.h"

#include "UIComponent.h"
#include "SceneManager.hpp"

#include "stb_image.h"
#define VOXEL_GRID_SIZE_X 160
#define VOXEL_GRID_SIZE_Y 90
#define VOXEL_GRID_SIZE_Z 128
#define NUM_BLUE_NOISE_TEXTURES 16
#define SHADOW_WIDTH 4096
#define SHADOW_HEIGHT 4096
#define POINT_SHADOW_SIZE 1024
#define MAX_POINT_LIGHTS 64
#define MAX_SHADOW_CASTING_LIGHTS 4
#define MAX_SHADOW_CASTING_SPOTLIGHTS 4

#if defined(DEBUG) && defined(PLATFORM_WINDOWS)
#include "System_LevelEditor.hpp"
class LevelEditor; // forward declaration
#endif

#if defined(__ANDROID__)
// On Android, we must use clock_gettime
#include <time.h>

static inline double get_native_time() {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return (double)now.tv_sec + (double)now.tv_nsec / 1e9;
}

#else
// On other platforms (Windows, Linux), we can use GLFW
#include <GLFW/glfw3.h> // Make sure you have this include

static inline double get_native_time() {
    return glfwGetTime();
}
#endif

namespace EngineFramework
{

    GraphicsSystem* this_graphics_ptr = nullptr;

    GraphicsSystem::GraphicsSystem()
    {
        this_graphics_ptr = this;
    }

    GraphicsSystem::~GraphicsSystem()
    {
#ifdef PLATFORM_WINDOWS
        // Clean up GLFW
        glfwTerminate();
#endif
    }

    void GraphicsSystem::Initialize()
    {
        SetupGLFW();
        SetupGraphics();

        // Listen for events
        event_dispatcher.sink<Event_Level_Editor_Toggled>().connect<&GraphicsSystem::HANDLE_EVENT_LevelEditorToggled>(this);
        event_dispatcher.sink<Event_Window_Resized>().connect<&GraphicsSystem::HANDLE_EVENT_WindowResized>(this);
    }

    void GraphicsSystem::SetupGLFW()
    {
#ifdef PLATFORM_WINDOWS
        // Initialize GLFW library
        if (!glfwInit()) {
            throw std::runtime_error("Failed to initialize GLFW");
        }

        // Configure GLFW for OpenGL ES 3.2 on desktop
        glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);
        glfwWindowHint(GLFW_DEPTH_BITS, 24);

        pWindow = glfwCreateWindow(WIDTH, HEIGHT, "Framework", NULL, NULL);

        if (!pWindow) {
            glfwTerminate();
            throw std::runtime_error("Failed to create GLFW window");
        }

        glfwMakeContextCurrent(pWindow);
        glfwSwapInterval(1); // Enable vsync

        // Initialize GLAD (must happen after window context is created)
        if (!gladLoadGLES2Loader((GLADloadproc)glfwGetProcAddress)) {
            glfwTerminate();
            throw std::runtime_error("Failed to initialize GLAD");
        }

        InputManager::RegisterCallbacks(pWindow);
#endif

        // Print OpenGL info
        LOGI("OpenGL Version: %s", glGetString(GL_VERSION));
        LOGI("GLSL Version: %s", glGetString(GL_SHADING_LANGUAGE_VERSION));
        LOGI("Rendered by: %s", glGetString(GL_RENDERER));

        LOGI("Registering InputManager callbacks to GraphicsSystem window.");
    }

    void GraphicsSystem::SetupGraphics()
    {
        using AssetMgr = EngineFramework::AssetManagerSystem;

        // Create the main framebuffer for final output
        FBO tempFBO(FBO::Texture, WIDTH, HEIGHT, &sceneTextureID);
        mainFBO = std::move(tempFBO);

        // Create the shadow map framebuffer
        FBO tempShadowFBO(FBO::DepthTextureArray, SHADOW_WIDTH, SHADOW_HEIGHT, &m_csmTextureArray, CASCADE_COUNT);
        m_shadowFBO = std::move(tempShadowFBO);

        // Create the shadow map framebuffer
        FBO tempSpotShadowFBO(FBO::DepthTextureArray, SHADOW_WIDTH, SHADOW_HEIGHT, &m_spotTextureArray, MAX_SHADOW_CASTING_SPOTLIGHTS);
        m_spotShadowFBO = std::move(tempSpotShadowFBO);

        //glBindTexture(GL_TEXTURE_2D_ARRAY, m_spotTextureArray);

        //// 1. Enable Depth Comparison (Required for sampler2DArrayShadow)
        //glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
        //glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL); // or GL_LESS

        //// 2. Set Border Color to WHITE (Far Depth)
        //// This ensures that areas *outside* the spotlight cone are treated as "Far Away" (Unshadowed)
        //glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        //glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        //float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
        //glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, borderColor);

        //glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

        // Create the point shadow map framebuffer (we use default and setup manually as we need multiple cube map faces)
        FBO tempPointShadowFBO(FBO::Default, POINT_SHADOW_SIZE, POINT_SHADOW_SIZE);
        m_pointShadowFBO = std::move(tempPointShadowFBO);

        // Create framebuffer handle for point shadows
        glGenFramebuffers(1, &m_pointShadowFBO.handle);
        glBindFramebuffer(GL_FRAMEBUFFER, m_pointShadowFBO.handle);

        // Explicitly tell OpenGL this FBO has no color buffers
        GLenum drawBuffers[] = { GL_NONE };
        glDrawBuffers(1, drawBuffers);
        glReadBuffer(GL_NONE);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // Create point shadow map cube textures
        m_pointShadowMapTextures.resize(MAX_SHADOW_CASTING_LIGHTS);
        glGenTextures(MAX_SHADOW_CASTING_LIGHTS, m_pointShadowMapTextures.data());

        for (int i = 0; i < MAX_SHADOW_CASTING_LIGHTS; i++) {
            glBindTexture(GL_TEXTURE_CUBE_MAP, m_pointShadowMapTextures[i]);
            for (unsigned int face = 0; face < 6; ++face) {
                glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, GL_DEPTH_COMPONENT24,
                    POINT_SHADOW_SIZE, POINT_SHADOW_SIZE, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, NULL);
            }
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
        }

        // Get shader GUIDs 
        mesh_shader_guid = AssetMgr::GetFullGuid(AssetMgr::SHADER_GUID, "Mesh");
        skybox_shader_guid = AssetMgr::GetFullGuid(AssetMgr::SHADER_GUID, "Skybox");
        shadow_shader_guid = AssetMgr::GetFullGuid(AssetMgr::SHADER_GUID, "Shadow");
        light_injection_shader_guid = AssetMgr::GetFullGuid(AssetMgr::SHADER_GUID, "LightInjection");
        ray_march_shader_guid = AssetMgr::GetFullGuid(AssetMgr::SHADER_GUID, "RayMarch");
        point_shadow_shader_guid = AssetMgr::GetFullGuid(AssetMgr::SHADER_GUID, "PointShadow");
        ui_shader_guid = AssetMgr::GetFullGuid(AssetMgr::SHADER_GUID, "UI");	
        
        // Create SSBO for point lights
        glGenBuffers(1, &m_pointLightSSBO);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_pointLightSSBO);
        glBufferData(GL_SHADER_STORAGE_BUFFER,
            sizeof(PointLightSSBO) * MAX_POINT_LIGHTS,
            nullptr, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

        // Initialize skinning UBO (for skeletal animation)
        glGenBuffers(1, &skinningUBO);
        glBindBuffer(GL_UNIFORM_BUFFER, skinningUBO);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(glm::mat4) * maxBones, nullptr, GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_UNIFORM_BUFFER, 3, skinningUBO); // binding = 0 in shader
        glBindBuffer(GL_UNIFORM_BUFFER, 0);

        // Create default textures
        glGenTextures(1, &m_defaultWhiteTexture);
        glBindTexture(GL_TEXTURE_2D, m_defaultWhiteTexture);
        unsigned char white[] = { 255, 255, 255, 255 };
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, white);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        LOGI("Created default white texture: %u", m_defaultWhiteTexture);

        glGenTextures(1, &m_defaultNormalTexture);
        glBindTexture(GL_TEXTURE_2D, m_defaultNormalTexture);
        unsigned char normal[] = { 128, 128, 255, 255 }; // RGB = (0, 0, 1) in tangent space
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, normal);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        LOGI("Created default normal texture: %u", m_defaultNormalTexture);

        glGenTextures(1, &m_defaultBlackTexture);
        glBindTexture(GL_TEXTURE_2D, m_defaultBlackTexture);
        unsigned char black[] = { 0, 0, 0, 255 }; // R=0, G=0, B=0, A=1
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, black);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glBindTexture(GL_TEXTURE_2D, 0);

        // Volumetric UBO
        glGenBuffers(1, &m_volumetricUBO);
        glBindBuffer(GL_UNIFORM_BUFFER, m_volumetricUBO);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(VolumetricUBO), nullptr, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);

        // Volumetric fog 3D textures
        glGenTextures(1, &m_ray_march_voxel_grid);
        glBindTexture(GL_TEXTURE_3D, m_ray_march_voxel_grid);
        glTexStorage3D(GL_TEXTURE_3D, 1, GL_RGBA16F, VOXEL_GRID_SIZE_X, VOXEL_GRID_SIZE_Y, VOXEL_GRID_SIZE_Z);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

        glGenTextures(2, m_temporal_integration_voxel_grid);
        for (int i = 0; i < 2; i++)
        {
            glBindTexture(GL_TEXTURE_3D, m_temporal_integration_voxel_grid[i]);
            glTexStorage3D(GL_TEXTURE_3D, 1, GL_RGBA16F, VOXEL_GRID_SIZE_X, VOXEL_GRID_SIZE_Y, VOXEL_GRID_SIZE_Z);
            glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        }
        glBindTexture(GL_TEXTURE_3D, 0);

        LoadBlueNoiseTextures();
        PreGeneratePrimitives();
    }

    void GraphicsSystem::Update(float deltaTime)
    {
        UNREFERENCED_PARAMETER(deltaTime);
        RenderScene();
    }

    void GraphicsSystem::PostUpdate(float deltaTime)
    {
        UNREFERENCED_PARAMETER(deltaTime);
#ifdef PLATFORM_WINDOWS
        glfwSwapBuffers(pWindow);
#endif
    }

    void GraphicsSystem::RenderScene()
    {
        glEnable(GL_DEPTH_TEST);

        auto active_camera_entity = GetFirstActiveCamera();
        if (active_camera_entity == entt::null) {
            LOGI("No cameras in registry yet, skipping render");
            return;
        }

        CameraComponent* active_camera = &registry.get<CameraComponent>(active_camera_entity);
        glm::mat4 V = Camera::getLookAt(*active_camera);
        float aspect = (screen.y > 0) ? (float)screen.x / screen.y : 1.0f;
        glm::mat4 P = Camera::getPerspective(*active_camera, aspect);

        // Update all model matrices
        auto transform_view = registry.view<TransformComponent>();
        for (auto [entity, transform] : transform_view.each()) {
            CalculateModelMatrix(transform, transform.modelMatrix);
        }

        glm::mat4 viewProj = P * V;

        // 1. Update point light SSBO
        UpdatePointLightSSBO(*active_camera, viewProj);
        // 2. Shadow pass (directional + point)
        ShadowPass();
        // 3. Global uniform buffers (camera, light, cascades)
        UpdateUBOs(*active_camera, V, P);
        // 4. Volumetric light injection
        VolumetricLightInjectionPass();
        // 5. Volumetric ray march
        VolumetricRayMarchPass();
        // 6. Mesh pass
        MeshPass(viewProj);
        // 7. Skybox pass
        SkyboxPass();
		// 8. UI pass
		RenderUI();
        // 9. Copy to screen if we are not in level editor
        if (!m_isRenderingToFBO) {
            glBindFramebuffer(GL_READ_FRAMEBUFFER, mainFBO.handle);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
            glBlitFramebuffer(0, 0, (GLint)screen.x, (GLint)screen.y,
                              0, 0, (GLint)screen.x, (GLint)screen.y,
                              GL_COLOR_BUFFER_BIT, GL_NEAREST);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }

        m_prev_view_projection = viewProj;
        m_frame_idx++;
    }

    void GraphicsSystem::UpdatePointLightSSBO(const CameraComponent& camera, const glm::mat4& viewProj)
    {
        m_pointLightData.clear();
        lightDistances.clear();
        m_lightEntityMap.clear();

        // Collect point and spot lights
        for (auto [entity, light] : registry.view<LightComponent>().each()) {
            bool isValidType = (light.type == LightComponent::POINT || light.type == LightComponent::SPOT);

            if (isValidType && m_pointLightData.size() < MAX_POINT_LIGHTS) {
                PointLightSSBO data;
                data.position_radius = glm::vec4(light.position, light.radius);
                data.color_intensity = glm::vec4(light.color * light.intensity, 0.0f);

                // Pack type into .w: 1 = point, 2 = spot
                float typeVal = (light.type == LightComponent::SPOT) ? 2.0f : 1.0f;
                data.attenuation_type = glm::vec4(light.constant, light.linear, light.quadratic, typeVal);

                // Direction in .xyz, default shadow index in .w
                data.direction_shadow = glm::vec4(light.direction, -1.0f);

                // Spot light cutoffs stored in .x and .y, default -1 for point lights
                float inner = -1.0f;
                float outer = -1.0f;
                if (light.type == LightComponent::SPOT) {
                    inner = glm::cos(glm::radians(light.innerConeAngle));
                    outer = glm::cos(glm::radians(light.outerConeAngle));
                }
                data.spot_cutoffs = glm::vec4(inner, outer, 0.0f, 0.0f);

                // Distance from camera for shadow priority
                float dist = glm::length(camera.position - light.position);

                // Only consider lights in camera frustum for shadows
                if (isLightInFrustum(light.position, light.radius, viewProj)) {
                    lightDistances.push_back({ static_cast<int>(m_pointLightData.size()), dist });
                }
                m_lightEntityMap.push_back(entity);
                m_pointLightData.push_back(data);
            }
        }

        // Sort visible lights by distance (nearest get shadows)
        std::sort(lightDistances.begin(), lightDistances.end(),
            [](const auto& a, const auto& b) { return a.second < b.second; });

        // Assign shadow map indices
        int pointShadowCount = 0;
        int spotShadowCount = 0;

        for (const auto& pair : lightDistances) {
            int lightIndex = pair.first;
            float type = m_pointLightData[lightIndex].attenuation_type.w;

            // Get the actual component to update its internal state
            //entt::entity e = m_lightEntityMap[lightIndex];
            //auto& lightComp = registry.get<LightComponent>(e);

            int assignedIndex = -1;

            if (type == 2.0f) { // Spot
                if (spotShadowCount < MAX_SHADOW_CASTING_LIGHTS) {
                    assignedIndex = spotShadowCount++;
                }
            }
            else { // Point
                if (pointShadowCount < MAX_SHADOW_CASTING_LIGHTS) {
                    assignedIndex = pointShadowCount++;
                }
            }

            // Apply Index
            m_pointLightData[lightIndex].direction_shadow.w = (float)assignedIndex;

            // dirty flag setting removed for now
            // CRITICAL: If the assigned slot changed (e.g., 0 -> 1), we MUST render 
            // because Texture 1 contains garbage, even if the light itself didn't move.
            //if (lightComp._lastShadowIndex != assignedIndex) {
            //    lightComp._isDirty = true; // Force redraw
            //    lightComp._lastShadowIndex = assignedIndex;
            //}
        }
        /*int shadowCount = std::min(MAX_SHADOW_CASTING_LIGHTS, (int)lightDistances.size());
        for (int i = 0; i < shadowCount; i++) {
            int lightIndex = lightDistances[i].first;
            m_pointLightData[lightIndex].direction_shadow.w = float(i);
        }*/

        // Upload to SSBO
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_pointLightSSBO);
        glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0,
            sizeof(PointLightSSBO) * m_pointLightData.size(),
            m_pointLightData.data());
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }

    void GraphicsSystem::RenderDirectionalShadow()
    {
        LightComponent* dirLight = nullptr;
        for (auto [entity, light] : registry.view<LightComponent>().each()) {
            if (light.type == LightComponent::DIRECTIONAL) {
                dirLight = &light;
                break;
            }
        }

        if (!dirLight) return;

        auto active_camera_entity = GetFirstActiveCamera();
        if (active_camera_entity == entt::null) return;

        CameraComponent& camera = registry.get<CameraComponent>(active_camera_entity);
        glm::mat4 camView = Camera::getLookAt(camera);

        // Cascade splits
        CalculateCascadeSplits(camera.nearPlane, camera.farPlane);

        std::shared_ptr<ShaderProgram> shader{};
        if (shadow_shader_guid != xresource::full_guid{})
        {
            shader = EngineFramework::AssetManagerSystem::GetResource<ShaderProgram>(shadow_shader_guid);
        }
        else
        {
            using E_AM = EngineFramework::AssetManagerSystem;
            shadow_shader_guid = E_AM::GetFullGuid(E_AM::SHADER_GUID, "Shadow");
            shader = E_AM::GetResource<ShaderProgram>(shadow_shader_guid);
        }

        shader->use();

        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_FRONT);

        // One pass per cascade
        for (int cascade = 0; cascade < CASCADE_COUNT; cascade++) {
            glBindFramebuffer(GL_FRAMEBUFFER, m_shadowFBO.handle);
            glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                m_csmTextureArray, 0, cascade);
            glClear(GL_DEPTH_BUFFER_BIT);

            float cascadeBias = m_shadow_bias * (1.0f + float(cascade) * 0.5f);
            m_cascadeBiases[cascade] = cascadeBias;

            float nearSplit = m_cascadeSplits[cascade];
            float farSplit = m_cascadeSplits[cascade + 1];

            // Frustum slice corners
            std::vector<glm::vec3> frustumCorners = GetFrustumCornersWorldSpace(
                camera, camView, nearSplit, farSplit);

            // Average center of corners
            glm::vec3 center(0.0f);
            for (const auto& corner : frustumCorners) {
                center += corner;
            }
            center /= frustumCorners.size();

            glm::vec3 lightDir = dirLight->direction;

            // Stable up vector
            glm::vec3 up = glm::abs(lightDir.y) > 0.999f ? glm::vec3(1, 0, 0) : glm::vec3(0, 1, 0);

            // Light view, anchored at origin for stability
            glm::mat4 lightView = glm::lookAt(glm::vec3(0.0f), lightDir, up);

            // Bounding sphere of frustum slice
            glm::vec3 frustumCenter(0.0f);
            for (const auto& corner : frustumCorners) {
                frustumCenter += corner;
            }
            frustumCenter /= frustumCorners.size();

            float radius = 0.0f;
            for (const auto& corner : frustumCorners) {
                float dist = glm::length(corner - frustumCenter);
                radius = glm::max(radius, dist);
            }

            glm::vec3 lightSpaceCenter = glm::vec3(lightView * glm::vec4(frustumCenter, 1.0f));

            float diameter = radius * 2.0f;
            float texelSize = diameter / SHADOW_WIDTH;

            // Snap center to texel grid to reduce shimmering
            lightSpaceCenter.x = floor(lightSpaceCenter.x / texelSize) * texelSize;
            lightSpaceCenter.y = floor(lightSpaceCenter.y / texelSize) * texelSize;

            float minX = lightSpaceCenter.x - radius;
            float maxX = lightSpaceCenter.x + radius;
            float minY = lightSpaceCenter.y - radius;
            float maxY = lightSpaceCenter.y + radius;

            float zMult = 10.0f;
            float minZ = lightSpaceCenter.z - radius * zMult;
            float maxZ = lightSpaceCenter.z + radius * zMult;

            // Ortho projection in light space
            glm::mat4 lightProj = glm::ortho(minX, maxX, minY, maxY, -maxZ, -minZ);

            m_cascadeViewProjMatrices[cascade] = lightProj * lightView;

            shader->setUniform("u_LightMatrix", m_cascadeViewProjMatrices[cascade]);

            /*float minSize = 0.0f;
            if (cascade == 1) minSize = 2.0f;
            if (cascade == 2) minSize = 5.0f;
            if (cascade == 3) minSize = 10.0f;*/

            auto view = registry.view<MeshComponent, TransformComponent>();
            for (auto [entity, meshComp, transformComp] : view.each()) {
                if (meshComp.meshName == "SkyBox") continue;
                if (!meshComp.visibility) continue; // Basic check

                //float maxScale = std::max(transformComp.scale.x, std::max(transformComp.scale.y, transformComp.scale.z));
                // Assuming average mesh size is ~1 unit. If you have tiny meshes scaled up, this works fine.
                //if (maxScale < minSize) continue;

                // --- THE FIX: Cull against the Shadow Cascade Matrix ---
                if (!IsBoxInFrustum(meshComp.localMin, meshComp.localMax, transformComp.modelMatrix, m_cascadeViewProjMatrices[cascade])) {
                    continue;
                }
                xresource::full_guid mesh_guid = AssetManagerSystem::GetFullGuid(
                    AssetManagerSystem::MESH_GUID, meshComp.meshName);
                auto meshData = AssetManagerSystem::GetResource<Mesh::MeshData>(mesh_guid);

                if (!meshData || meshData->vao == 0) continue;

                shader->setUniform("u_Model", transformComp.modelMatrix);

                glBindVertexArray(meshData->vao);
                for (const auto& subMesh : meshData->subMeshes) {
                    glDrawElements(GL_TRIANGLES, subMesh.indexCount, GL_UNSIGNED_INT,
                        (void*)(sizeof(unsigned int) * subMesh.baseIndex));
                }
            }
        }

        glCullFace(GL_BACK);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        shader->disuse();
        glViewport(0, 0, (GLsizei)screen.x, (GLsizei)screen.y);
    }

    void GraphicsSystem::RenderShadowCubemap(int cubemapSlot, const glm::vec4& lightPosRadius)
    {
        glm::vec3 lightPos = glm::vec3(lightPosRadius);
        float lightRadius = lightPosRadius.w;

        // --- Pre-Filter Casters ---
        // Instead of looping 10,000 entities * 6 faces = 60,000 checks
        // We filter once against the light sphere to get ~50 valid objects.
        struct ShadowCaster {
            entt::entity entity; // Keep entity ID if needed for other lookups
            const MeshComponent* mesh;
            const TransformComponent* transform;
        };
        // Static vector to reuse memory across frames (prevents allocation overhead)
        static std::vector<ShadowCaster> validCasters;
        validCasters.clear();

        auto view = registry.view<MeshComponent, TransformComponent>();
        for (auto [entity, meshComp, transformComp] : view.each()) {
            if (meshComp.meshName == "SkyBox" || !meshComp.visibility) continue;

            // 1. Calculate World Center & Radius
            glm::vec3 minV = glm::vec3(meshComp.localMin[0], meshComp.localMin[1], meshComp.localMin[2]);
            glm::vec3 maxV = glm::vec3(meshComp.localMax[0], meshComp.localMax[1], meshComp.localMax[2]);
            glm::vec3 center = (minV + maxV) * 0.5f;
            glm::vec3 extents = (maxV - minV) * 0.5f;

            const glm::mat4& model = transformComp.modelMatrix;
            glm::vec3 worldCenter = glm::vec3(model * glm::vec4(center, 1.0f));

            // Fast approximation of world radius (scale * max_extent)
            float maxScale = std::max(transformComp.scale.x, std::max(transformComp.scale.y, transformComp.scale.z));
            float radius = glm::length(extents) * maxScale;

            // 2. Sphere-Sphere Intersection (Light vs Object)
            float dist = glm::distance(worldCenter, lightPos);
            if (dist > lightRadius + radius) continue; // Too far from light

            validCasters.push_back({ entity, &meshComp, &transformComp });
        }

        // --- SETUP RENDER ---
        glViewport(0, 0, POINT_SHADOW_SIZE, POINT_SHADOW_SIZE);
        glBindFramebuffer(GL_FRAMEBUFFER, m_pointShadowFBO.handle);
        glEnable(GL_DEPTH_TEST);
        glCullFace(GL_FRONT); // Cull Front faces for shadows to prevent self-shadowing acne

        glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, lightRadius);

        // Recalculate views (standard)
        glm::mat4 shadowViews[6] = {
            glm::lookAt(lightPos, lightPos + glm::vec3(1, 0, 0), glm::vec3(0,-1, 0)),
            glm::lookAt(lightPos, lightPos + glm::vec3(-1, 0, 0), glm::vec3(0,-1, 0)),
            glm::lookAt(lightPos, lightPos + glm::vec3(0, 1, 0), glm::vec3(0, 0, 1)),
            glm::lookAt(lightPos, lightPos + glm::vec3(0,-1, 0), glm::vec3(0, 0,-1)),
            glm::lookAt(lightPos, lightPos + glm::vec3(0, 0, 1), glm::vec3(0,-1, 0)),
            glm::lookAt(lightPos, lightPos + glm::vec3(0, 0,-1), glm::vec3(0,-1, 0))
        };

        // Get Shader
        std::shared_ptr<ShaderProgram> shader{};
        if (point_shadow_shader_guid != xresource::full_guid{}) {
            shader = AssetManagerSystem::GetResource<ShaderProgram>(point_shadow_shader_guid);
        }
        else {
            using E_AM = AssetManagerSystem;
            point_shadow_shader_guid = E_AM::GetFullGuid(E_AM::SHADER_GUID, "PointShadow");
            shader = E_AM::GetResource<ShaderProgram>(point_shadow_shader_guid);
        }
        if (!shader) return;

        shader->use();
        shader->setUniform("u_LightPos", lightPos);
        shader->setUniform("u_FarPlane", lightRadius);

        // --- RENDER FACES ---
        for (int face = 0; face < 6; face++) {
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                GL_TEXTURE_CUBE_MAP_POSITIVE_X + face,
                m_pointShadowMapTextures[cubemapSlot], 0);
            glClear(GL_DEPTH_BUFFER_BIT);

            glm::mat4 lightMatrix = shadowProj * shadowViews[face];
            shader->setUniform("u_LightMatrix", lightMatrix);

            // Loop through our PRE-FILTERED list
            for (const auto& caster : validCasters) {

                // 3. Frustum Culling (Face vs Object)
                // Still needed because an object might touch the Light Sphere but be behind the current face
                if (!IsBoxInFrustum(caster.mesh->localMin, caster.mesh->localMax, caster.transform->modelMatrix, lightMatrix)) {
                    continue;
                }

                // Draw
                xresource::full_guid mesh_guid = AssetManagerSystem::GetFullGuid(
                    AssetManagerSystem::MESH_GUID, caster.mesh->meshName);
                auto meshData = AssetManagerSystem::GetResource<Mesh::MeshData>(mesh_guid);
                if (!meshData || meshData->vao == 0) meshData = meshCache[caster.mesh->meshName];
                if (!meshData || meshData->vao == 0) continue;

                shader->setUniform("u_Model", caster.transform->modelMatrix);
                glBindVertexArray(meshData->vao);
                for (const auto& subMesh : meshData->subMeshes) {
                    glDrawElements(GL_TRIANGLES, subMesh.indexCount, GL_UNSIGNED_INT,
                        (void*)(sizeof(unsigned int) * subMesh.baseIndex));
                }
            }
        }

        shader->disuse();
        glCullFace(GL_BACK);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, (GLsizei)screen.x, (GLsizei)screen.y);
    }

    void GraphicsSystem::RenderPointLightShadows()
    {
        for (size_t i = 0; i < m_pointLightData.size(); i++) {
            const auto& lightData = m_pointLightData[i];

            // 1. Check if it has a shadow index
            if (lightData.direction_shadow.w < 0.0f) continue;

            // 2. Retrieve the component
            entt::entity e = m_lightEntityMap[i];
            auto& lightComp = registry.get<LightComponent>(e);

            // 3. THE OPTIMIZATION
            // If Static AND Clean, skip all 6 draw calls!
            if (lightComp.isStatic && !lightComp._isDirty) {
                continue;
            }

            // 4. Render
            int cubemapSlot = int(lightData.direction_shadow.w);
            RenderShadowCubemap(cubemapSlot, lightData.position_radius);

            // 5. Mark Clean
            if (lightComp.isStatic) {
                lightComp._isDirty = false;
            }
        }
    }

    void GraphicsSystem::RenderSpotLightShadows() {
        // 1. Get and Use Shader
        std::shared_ptr<ShaderProgram> shader{};
        if (shadow_shader_guid != xresource::full_guid{}) {
            shader = AssetManagerSystem::GetResource<ShaderProgram>(shadow_shader_guid);
        }
        else {
            using E_AM = AssetManagerSystem;
            shader = E_AM::GetResource<ShaderProgram>(E_AM::GetFullGuid(E_AM::SHADER_GUID, "Shadow"));
        }

        if (!shader) return;
        shader->use();

        // 2. Setup Framebuffer
        glBindFramebuffer(GL_FRAMEBUFFER, m_spotShadowFBO.handle);
        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);

        // 3. Setup Render State
        glEnable(GL_CULL_FACE);
        glCullFace(GL_FRONT); // Front-face culling for shadows

        // 4. Reset Matrices
        // Since we use std::array, we just fill it with identity matrices.
        m_spotViewProjMatrices.fill(glm::mat4(1.0f));

        // 5. Iterate through sorted light data
        // Note: We use the shadow index stored in .w to determine which array slot to use
        for (size_t i = 0; i < m_pointLightData.size(); i++)
        {
            const auto& lightData = m_pointLightData[i];

            // Skip if not Spot Light (Type 2.0) or has no shadow index assigned
            if (lightData.attenuation_type.w < 1.5f || lightData.direction_shadow.w < 0.0f) {
                continue;
            }

            // The shadow index (0 to 3) we assigned in UpdatePointLightSSBO
            int shadowIndex = (int)lightData.direction_shadow.w;

            // Safety check to ensure we don't write out of bounds
            if (shadowIndex >= MAX_SHADOW_CASTING_LIGHTS) continue;

            // --- Calculate Matrix ---
            glm::vec3 pos = glm::vec3(lightData.position_radius);
            glm::vec3 dir = glm::vec3(lightData.direction_shadow); // .xyz is direction
            float range = lightData.position_radius.w;

            // Use a wide FOV (e.g., 90 degrees) to cover the spotlight cone
            // 1. Get cosine value (e.g., 0.9)
            float outerCos = lightData.spot_cutoffs.y;

            // 2. Convert to radians (Half Angle) -> acos(0.9) = ~25 degrees
            float outerAngleRad = std::acos(outerCos);

            // 3. Multiply by 2 for full Field of View -> 50 degrees
            float fov = outerAngleRad * 2.0f;

            // 4. Safety clamp (prevent crash if acos fails or angle is too wide)
            fov = glm::clamp(fov, 0.1f, glm::radians(170.0f));

            glm::mat4 proj = glm::perspective(fov, 1.0f, 0.1f, range);

            // Safe Up Vector
            glm::vec3 up = (std::abs(dir.y) > 0.999f) ? glm::vec3(1, 0, 0) : glm::vec3(0, 1, 0);

            glm::mat4 view = glm::lookAt(pos, pos + dir, up);
            glm::mat4 lightSpaceMatrix = proj * view;

            // Store in our fixed array
            m_spotViewProjMatrices[shadowIndex] = lightSpaceMatrix;

            // --- Render to Depth Texture Array Layer ---
            // Access the specific layer (shadowIndex) of the texture array
            glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                m_spotTextureArray, 0, shadowIndex);

            glClear(GL_DEPTH_BUFFER_BIT);

            // Upload uniform for the vertex shader
            shader->setUniform("u_LightMatrix", lightSpaceMatrix);

            // --- Draw Scene ---
            auto meshView = registry.view<MeshComponent, TransformComponent>();
            for (auto [entity, meshComp, transformComp] : meshView.each())
            {
                if (meshComp.meshName == "SkyBox" || !meshComp.visibility) continue;

                // --- THE FIX: Cull against the Shadow Cascade Matrix ---
                if (!IsBoxInFrustum(meshComp.localMin, meshComp.localMax, transformComp.modelMatrix, lightSpaceMatrix)) {
                    continue;
                }
                xresource::full_guid mesh_guid = AssetManagerSystem::GetFullGuid(
                    AssetManagerSystem::MESH_GUID, meshComp.meshName);
                auto meshData = AssetManagerSystem::GetResource<Mesh::MeshData>(mesh_guid);

                if (!meshData || meshData->vao == 0) meshData = meshCache[meshComp.meshName];
                if (!meshData || meshData->vao == 0) continue;

                shader->setUniform("u_Model", transformComp.modelMatrix);

                glBindVertexArray(meshData->vao);
                for (const auto& subMesh : meshData->subMeshes) {
                    glDrawElements(GL_TRIANGLES, subMesh.indexCount, GL_UNSIGNED_INT,
                        (void*)(sizeof(unsigned int) * subMesh.baseIndex));
                }
            }
        }

        // 6. Cleanup
        glCullFace(GL_BACK);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        shader->disuse();
        glViewport(0, 0, (GLsizei)screen.x, (GLsizei)screen.y);
    }

    void GraphicsSystem::ShadowPass()
    {
        RenderDirectionalShadow();
        RenderPointLightShadows();
		RenderSpotLightShadows();
    }

    void GraphicsSystem::UpdateUBOs(const CameraComponent& camera, const glm::mat4& V, const glm::mat4& P)
    {
        glBindBuffer(GL_UNIFORM_BUFFER, m_volumetricUBO);
        VolumetricUBO* ubo = (VolumetricUBO*)glMapBufferRange(
            GL_UNIFORM_BUFFER, 0, sizeof(VolumetricUBO), GL_MAP_WRITE_BIT);

        LightComponent* dirLight = nullptr;
        for (auto [entity, light] : registry.view<LightComponent>().each()) {
            if (light.type == LightComponent::DIRECTIONAL) {
                dirLight = &light;
                break;
            }
        }

        glm::vec3 direction = glm::vec3(0, -1, 0);
        glm::vec3 color = glm::vec3(1.0f);
        float intensity = 0.0f;

        if (dirLight) {
            direction = dirLight->direction;
            color = dirLight->color;
            intensity = dirLight->intensity;
        }

        ubo->view = V;
        ubo->projection = P;
        ubo->view_proj = P * V;
        ubo->prev_view_proj = m_frame_idx == 0 ? P * V : m_prev_view_projection;
        ubo->inv_view_proj = glm::inverse(ubo->view_proj);

        for (int i = 0; i < MAX_SHADOW_CASTING_LIGHTS; i++) {
            ubo->spot_view_proj[i] = m_spotViewProjMatrices[i];
        }

        for (int i = 0; i < CASCADE_COUNT; i++) {
            ubo->cascade_view_proj[i] = m_cascadeViewProjMatrices[i];
        }

        ubo->cascade_splits = glm::vec4(
            m_cascadeSplits[1],
            m_cascadeSplits[2],
            m_cascadeSplits[3],
            0.0f
        );

        ubo->cascade_biases = glm::vec4(
            m_cascadeBiases[0],
            m_cascadeBiases[1],
            m_cascadeBiases[2],
            m_cascadeBiases[3]
        );

        ubo->light_direction = glm::vec4(direction, 0.0f);
        ubo->light_color = glm::vec4(color * intensity, m_ambient_light_intensity);
        ubo->camera_position = glm::vec4(camera.position, 1.0f);
        ubo->bias_near_far_pow = glm::vec4(
            m_shadow_bias,
            camera.nearPlane,
            camera.farPlane,
            m_depth_power
        );
        ubo->aniso_density_scattering_absorption = glm::vec4(
            m_anisotropy,
            m_density,
            m_scattering,
            m_absorption
        );
        ubo->time = glm::vec4(static_cast<float>(get_native_time()), 0, 0, 0);
        ubo->width_height = glm::ivec4((int)screen.x, (int)screen.y, 0, 0);

        glUnmapBuffer(GL_UNIFORM_BUFFER);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

    void GraphicsSystem::VolumetricLightInjectionPass()
    {
        std::shared_ptr<ShaderProgram> shader{};
        if (light_injection_shader_guid != xresource::full_guid{})
        {
            shader = EngineFramework::AssetManagerSystem::GetResource<ShaderProgram>(light_injection_shader_guid);
        }
        else
        {
            using E_AM = EngineFramework::AssetManagerSystem;
            light_injection_shader_guid = E_AM::GetFullGuid(E_AM::SHADER_GUID, "LightInjection");
            shader = E_AM::GetResource<ShaderProgram>(light_injection_shader_guid);
        }

        shader->use();
        glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_volumetricUBO);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_pointLightSSBO);

        uint32_t read_idx = static_cast<uint32_t>(m_ping_pong);
        uint32_t write_idx = static_cast<uint32_t>(!m_ping_pong);

        glBindImageTexture(0, m_temporal_integration_voxel_grid[write_idx],
                           0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA16F);

        glActiveTexture(GL_TEXTURE0 + TextureUnits::VOLUMETRIC_HISTORY_UNIT);
        glBindTexture(GL_TEXTURE_3D, m_temporal_integration_voxel_grid[read_idx]);
        shader->setUniform("s_History", TextureUnits::VOLUMETRIC_HISTORY_UNIT);

        glActiveTexture(GL_TEXTURE0 + TextureUnits::VOLUMETRIC_BLUE_NOISE_UNIT);
        glBindTexture(GL_TEXTURE_2D, m_blue_noise_textures[m_temporal_accumulation ? m_frame_idx % NUM_BLUE_NOISE_TEXTURES : 0]);
        shader->setUniform("s_BlueNoise", TextureUnits::VOLUMETRIC_BLUE_NOISE_UNIT);

        glActiveTexture(GL_TEXTURE0 + TextureUnits::SHADOW_MAP_UNIT);
        glBindTexture(GL_TEXTURE_2D_ARRAY, m_csmTextureArray);
        shader->setUniform("s_CascadeShadowMaps", TextureUnits::SHADOW_MAP_UNIT);

        glActiveTexture(GL_TEXTURE0 + TextureUnits::SPOT_LIGHT_MAP_UNIT);
        glBindTexture(GL_TEXTURE_2D_ARRAY, m_spotTextureArray);
        shader->setUniform("s_SpotShadowMaps", TextureUnits::SPOT_LIGHT_MAP_UNIT);

        int baseUnit = TextureUnits::POINT_SHADOW_MAP_UNIT;
        for (int i = 0; i < MAX_SHADOW_CASTING_LIGHTS; i++) {
            glActiveTexture(GL_TEXTURE0 + baseUnit + i);
            glBindTexture(GL_TEXTURE_CUBE_MAP, m_pointShadowMapTextures[i]);

            std::string uniformName = "s_PointShadowCubemaps[" + std::to_string(i) + "]";
            shader->setUniform(uniformName.c_str(), baseUnit + i);
        }

        shader->setUniform("u_NumPointLights", (int)m_pointLightData.size());
        shader->setUniform("u_Accumulation", m_frame_idx > 0 && m_temporal_accumulation);

        glDispatchCompute((VOXEL_GRID_SIZE_X + 7) / 8,
                          (VOXEL_GRID_SIZE_Y + 7) / 8,
                          (VOXEL_GRID_SIZE_Z + 0) / 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

        m_ping_pong = !m_ping_pong;
        shader->disuse();
    }

    void GraphicsSystem::VolumetricRayMarchPass()
    {
        std::shared_ptr<ShaderProgram> shader{};
        if (ray_march_shader_guid != xresource::full_guid{})
        {
            shader = EngineFramework::AssetManagerSystem::GetResource<ShaderProgram>(ray_march_shader_guid);
        }
        else
        {
            using E_AM = EngineFramework::AssetManagerSystem;
            ray_march_shader_guid = E_AM::GetFullGuid(E_AM::SHADER_GUID, "RayMarch");
            shader = E_AM::GetResource<ShaderProgram>(ray_march_shader_guid);
        }

        shader->use();
        glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_volumetricUBO);
        glBindImageTexture(0, m_ray_march_voxel_grid, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA16F);

        uint32_t read_idx = static_cast<uint32_t>(m_ping_pong);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_3D, m_temporal_integration_voxel_grid[read_idx]);
        shader->setUniform("s_VoxelGrid", 0);

        glDispatchCompute((VOXEL_GRID_SIZE_X + 7) / 8,
                          (VOXEL_GRID_SIZE_Y + 7) / 8,
                          1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
        shader->disuse();
    }

    void GraphicsSystem::MeshPass(const glm::mat4& viewProj)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, mainFBO.handle);
        glViewport(0, 0, (GLsizei)screen.x, (GLsizei)screen.y);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        std::shared_ptr<ShaderProgram> shader{};
        if (mesh_shader_guid != xresource::full_guid{})
        {
            shader = EngineFramework::AssetManagerSystem::GetResource<ShaderProgram>(mesh_shader_guid);
        }
        else
        {
            using E_AM = EngineFramework::AssetManagerSystem;
            mesh_shader_guid = E_AM::GetFullGuid(E_AM::SHADER_GUID, "Mesh");
            shader = E_AM::GetResource<ShaderProgram>(mesh_shader_guid);
        }

        if (!shader) {
            LOGE("Mesh shader not found!");
			return;
        }

        shader->use();
        glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_volumetricUBO);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_pointLightSSBO);

        // Shadow maps
        glActiveTexture(GL_TEXTURE0 + TextureUnits::SHADOW_MAP_UNIT);
        glBindTexture(GL_TEXTURE_2D_ARRAY, m_csmTextureArray);
        shader->setUniform("s_CascadeShadowMaps", TextureUnits::SHADOW_MAP_UNIT);

        glActiveTexture(GL_TEXTURE0 + TextureUnits::SPOT_LIGHT_MAP_UNIT);
        glBindTexture(GL_TEXTURE_2D_ARRAY, m_spotTextureArray);
        shader->setUniform("s_SpotShadowMaps", TextureUnits::SPOT_LIGHT_MAP_UNIT);

        // Point light cubemaps
        int baseUnit = TextureUnits::POINT_SHADOW_MAP_UNIT;
        for (int i = 0; i < MAX_SHADOW_CASTING_LIGHTS; i++) {
            glActiveTexture(GL_TEXTURE0 + baseUnit + i);
            glBindTexture(GL_TEXTURE_CUBE_MAP, m_pointShadowMapTextures[i]);

            std::string uniformName = "s_PointShadowCubemaps[" + std::to_string(i) + "]";
            shader->setUniform(uniformName.c_str(), baseUnit + i);
        }

        // Volumetric scattering texture
        glActiveTexture(GL_TEXTURE0 + TextureUnits::VOLUMETRIC_VOXEL_GRID_UNIT);
        glBindTexture(GL_TEXTURE_3D, m_ray_march_voxel_grid);
        shader->setUniform("s_VoxelGrid", TextureUnits::VOLUMETRIC_VOXEL_GRID_UNIT);
        shader->setUniform("u_Tricubic", m_tricubic_filtering);

        shader->setUniform("u_NumPointLights", (int)m_pointLightData.size());

        // Debug modes via F1-F10
        /*if (SceneManager::GetInstance().is_file_loaded) {
            if (ImGui::IsKeyDown(ImGuiKey_F1)) debugMode = 0;
            if (ImGui::IsKeyDown(ImGuiKey_F2)) debugMode = 1;
            if (ImGui::IsKeyDown(ImGuiKey_F3)) debugMode = 2;
            if (ImGui::IsKeyDown(ImGuiKey_F4)) debugMode = 3;
            if (ImGui::IsKeyDown(ImGuiKey_F5)) debugMode = 4;
            if (ImGui::IsKeyDown(ImGuiKey_F6)) debugMode = 5;
            if (ImGui::IsKeyDown(ImGuiKey_F7)) debugMode = 6;
            if (ImGui::IsKeyDown(ImGuiKey_F8)) debugMode = 7;
            if (ImGui::IsKeyDown(ImGuiKey_F9)) debugMode = 8;
            if (ImGui::IsKeyDown(ImGuiKey_F10)) debugMode = 9;
        }
        shader->setUniform("u_DebugMode", debugMode);

        static int lastDebugMode = -1;
        if (debugMode != lastDebugMode) {
            const char* modeNames[] = {
                "Normal", "Cascade Colors", "Shadow UV", "Raw Shadow",
                "Depth Values", "Cascade Gradients", "Cascade 0 Only",
                "Light Direction", "Distance", "Cascade 0 Lit/Shadow"
            };
            LOGI("Debug Mode: %d - %s", debugMode, modeNames[debugMode]);
            lastDebugMode = debugMode;
        }*/

        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glDepthFunc(GL_LESS);

        auto view = registry.view<MeshComponent, TransformComponent>();

        // Reusable identity palette for non-skinned meshes
        static std::vector<glm::mat4> s_identityPalette;
		//int culledMeshes = 0;
        for (auto [entity, meshComp, transformComp] : view.each())
        {
            if (meshComp.meshName == "SkyBox") continue;
			if (!meshComp.visibility) continue;
            if (!IsBoxInFrustum(meshComp.localMin, meshComp.localMax, transformComp.modelMatrix, viewProj)) {
				//culledMeshes++;
                continue; // Mesh is off-screen!

            }
            // Upload bone palette  
            const std::vector<glm::mat4>* paletteToUse = nullptr;
            bool useSkinning = false;

            if (auto anim = registry.try_get<AnimationComponent>(entity))
            {
                if (anim->loaded && !anim->paletteMatrices.empty())
                {
                    paletteToUse = &anim->paletteMatrices;
                    useSkinning = true;
                }
            }

            if (!paletteToUse)
            {
                // Use identity palette for non-animated meshes or if anim not ready
                if (s_identityPalette.empty())
                    s_identityPalette.assign(maxBones, glm::mat4(1.0f));
                paletteToUse = &s_identityPalette;
                useSkinning = false;
            }

            UploadBonePalettes(*paletteToUse);

            xresource::full_guid mesh_guid = AssetManagerSystem::GetFullGuid(
                AssetManagerSystem::MESH_GUID, meshComp.meshName);

            auto meshData = AssetManagerSystem::GetResource<Mesh::MeshData>(mesh_guid);

            // Fall back to cached mesh if not a resource mesh
            if (!meshData || meshData->vao == 0) {
                meshData = meshCache[meshComp.meshName];
            }

            if (!meshData || meshData->vao == 0) continue;

            shader->setUniform("u_Model", transformComp.modelMatrix);

            glBindVertexArray(meshData->vao);

            const MaterialComponent* override_material = registry.try_get<MaterialComponent>(entity);

            for (const auto& subMesh : meshData->subMeshes)
            {
                if (subMesh.materialIndex < 0 ||
                    subMesh.materialIndex >= (int)meshData->materials.size())
                    continue;

                const MaterialComponent& material =
                    override_material ? *override_material : meshData->materials[subMesh.materialIndex];

                // Factors
                shader->setUniform("u_AlbedoFactor", material.albedoFactor);
                shader->setUniform("u_RoughnessFactor", material.roughnessFactor);
                shader->setUniform("u_MetallicFactor", material.metallicFactor);
                shader->setUniform("u_UVTransform", glm::vec4(
                    material.uvTiling.x,
                    material.uvTiling.y,
                    material.uvOffset.x,
                    material.uvOffset.y
                ));

                // Albedo
                auto albedoTex = AssetManagerSystem::GetResource<Scene::Texture>(material.albedoMap);
                glActiveTexture(GL_TEXTURE0 + TextureUnits::MESH_ALBEDO_UNIT);
                glBindTexture(GL_TEXTURE_2D, (albedoTex && albedoTex->id != 0) ? albedoTex->id : m_defaultWhiteTexture);
                shader->setUniform("s_Albedo", TextureUnits::MESH_ALBEDO_UNIT);

                // Normal
                auto normalTex = AssetManagerSystem::GetResource<Scene::Texture>(material.normalMap);
                glActiveTexture(GL_TEXTURE0 + TextureUnits::MESH_NORMAL_UNIT);
                glBindTexture(GL_TEXTURE_2D, (normalTex && normalTex->id != 0) ? normalTex->id : m_defaultNormalTexture);
                shader->setUniform("s_Normal", TextureUnits::MESH_NORMAL_UNIT);

                // Roughness
                auto roughTex = AssetManagerSystem::GetResource<Scene::Texture>(material.roughnessMap);
                glActiveTexture(GL_TEXTURE0 + TextureUnits::MESH_ROUGHNESS_UNIT);
                glBindTexture(GL_TEXTURE_2D, (roughTex && roughTex->id != 0) ? roughTex->id : m_defaultWhiteTexture);
                shader->setUniform("s_Roughness", TextureUnits::MESH_ROUGHNESS_UNIT);

                // Metallic
                auto metalTex = AssetManagerSystem::GetResource<Scene::Texture>(material.metallicMap);
                glActiveTexture(GL_TEXTURE0 + TextureUnits::MESH_METALLIC_UNIT);
                glBindTexture(GL_TEXTURE_2D, (metalTex && metalTex->id != 0) ? metalTex->id : m_defaultBlackTexture);
                shader->setUniform("s_Metallic", TextureUnits::MESH_METALLIC_UNIT);

                static bool logged_first = false;
                if (!logged_first) {
                    LOGI("=== First Submesh Render Debug ===");
                    LOGI("Submesh: %s", subMesh.name.c_str());
                    LOGI("Material: %s", material.name.c_str());
                    LOGI("  - Albedo: ptr=%p, id=%u, using_fallback=%d",
                        (void*)albedoTex.get(),
                        albedoTex ? albedoTex->id : 0,
                        !(albedoTex && albedoTex->id != 0));
                    LOGI("  - Normal: ptr=%p, id=%u, using_fallback=%d",
                        (void*)normalTex.get(),
                        normalTex ? normalTex->id : 0,
                        !(normalTex && normalTex->id != 0));
                    LOGI("  - Roughness: ptr=%p, id=%u, factor=%.2f",
                        (void*)roughTex.get(),
                        roughTex ? roughTex->id : 0,
                        material.roughnessFactor);
                    LOGI("  - Metallic: ptr=%p, id=%u, factor=%.2f",
                        (void*)metalTex.get(),
                        metalTex ? metalTex->id : 0,
                        material.metallicFactor);
                    LOGI("Default textures: white=%u, normal=%u", m_defaultWhiteTexture, m_defaultNormalTexture);
                    logged_first = true;
                }

                glDrawElements(GL_TRIANGLES, subMesh.indexCount,
                               GL_UNSIGNED_INT,
                               (void*)(sizeof(unsigned int) * subMesh.baseIndex));
            }
        }
		//LOGI("Culled meshes this frame: %d", culledMeshes);
        shader->disuse();
    }

    void GraphicsSystem::SkyboxPass()
    {
        std::shared_ptr<ShaderProgram> shader{};
        if (skybox_shader_guid != xresource::full_guid{})
        {
            shader = EngineFramework::AssetManagerSystem::GetResource<ShaderProgram>(skybox_shader_guid);
        }
        else
        {
            using E_AM = EngineFramework::AssetManagerSystem;
            skybox_shader_guid = E_AM::GetFullGuid(E_AM::SHADER_GUID, "SkyBox");
            shader = E_AM::GetResource<ShaderProgram>(skybox_shader_guid);
        }

        shader->use();
        glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_volumetricUBO);

        glDepthFunc(GL_LEQUAL);
        glDisable(GL_CULL_FACE);

        auto skybox_view = registry.view<MeshComponent, SkyboxRenderPass>();
        for (auto [entity, meshComp] : skybox_view.each()) {
            if (meshCache.find(meshComp.meshName) == meshCache.end())
                LoadAndCacheMesh(meshComp.meshName);

            auto& meshData = meshCache[meshComp.meshName];
            if (!meshData || meshData->vao == 0) continue;

            glBindVertexArray(meshData->vao);

            if (!meshData->subMeshes.empty()) {
                glDrawElements(GL_TRIANGLES, meshData->subMeshes[0].indexCount,
                               GL_UNSIGNED_INT, 0);
            }
            else {
                LOGE("Skybox mesh '%s' has no submeshes!", meshComp.meshName.c_str());
            }
        }

        glEnable(GL_CULL_FACE);
        glDepthFunc(GL_LESS);
        shader->disuse();
    }

    void GraphicsSystem::RenderUI() {
        // 1. Setup Orthographic Projection (0,0 is Bottom-Left, ScreenWidth,ScreenHeight is Top-Right)
        glm::mat4 projection = glm::ortho(0.0f, screen.x, 0.0f, screen.y);

        // 2. Prepare GL State for 2D
        glDisable(GL_DEPTH_TEST);    // UI is always on top
        glDisable(GL_CULL_FACE);     // Don't cull backfaces
        glEnable(GL_BLEND);          // Transparency is required for UI
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // 3. Get the UI Shader
        std::shared_ptr<ShaderProgram> shader{};
        if (ui_shader_guid != xresource::full_guid{})
        {
            shader = EngineFramework::AssetManagerSystem::GetResource<ShaderProgram>(ui_shader_guid);
        }
        else
        {
            using E_AM = EngineFramework::AssetManagerSystem;
            ui_shader_guid = E_AM::GetFullGuid(E_AM::SHADER_GUID, "UI");
            shader = E_AM::GetResource<ShaderProgram>(ui_shader_guid);
        }

        shader->use();
        shader->setUniform("u_Projection", projection);

        // 4. Get the Quad Mesh (We reuse the existing 3D quad)
        auto meshData = meshCache["Quad"];
        glBindVertexArray(meshData->vao);

        // 5. Iterate all UI Elements
        // We sort them by 'layer' so background panels draw before text/icons
        auto view = registry.view<UIComponent>();

        


        for (auto [entity, ui] : view.each())
        {
            if (!ui.visible) continue;

            if (ui.is_NDC) {

                ui.size.x = ui.size_NDC.x * screen.x;
                ui.size.y = ui.size_NDC.y * screen.y;

                ui.position.x = (ui.position_NDC.x + 1.0f) * 0.5f * screen.x;
                //ui.position.y = (1.0f - ui.position_NDC.y) * 0.5f * screen.y;
                ui.position.y = (ui.position_NDC.y + 1.0f) * 0.5f * screen.y;


                // shift from center to bottom-left anchor 
                // (because scale starts from bottom left up so like we need to shift down)
                ui.position.x -= ui.size.x * 0.5f;
                ui.position.y -= ui.size.y * 0.5f;
                

            }

            glm::vec2 finalPos = ui.position;
            glm::vec2 finalSize = ui.size;

            if (registry.all_of<TransformComponent>(entity)) {
                auto& transform = registry.get<TransformComponent>(entity);

                glm::vec2 screenPos;
                float scaleFactor = 1.0f;

                if (WorldToScreen(transform.position, screenPos, scaleFactor)) {
                    // 1. Calculate the Scaled Size based on the original ui.size
                    finalSize = ui.size * scaleFactor;

                    // 2. Center the position based on the NEW size
                    finalPos.x = screenPos.x - (finalSize.x * 0.5f);
                    finalPos.y = screenPos.y - (finalSize.y * 0.5f);;
                }
                else {
                    continue; // Skip rendering if not on screen
                }
            }

            


            // --- CALCULATE MODEL MATRIX ---
            glm::mat4 model = glm::mat4(1.0f);

            // Use 'finalPos' instead of 'ui.position'
            model = glm::translate(model, glm::vec3(finalPos.x, finalPos.y, 0.0f));

            // Use 'finalSize' instead of 'ui.size'
            model = glm::scale(model, glm::vec3(finalSize.x, finalSize.y, 1.0f));

            // --- COMPENSATION FOR -1 TO 1 MESH ---
            model = glm::translate(model, glm::vec3(0.5f, 0.5f, 0.0f));
            model = glm::scale(model, glm::vec3(0.5f, 0.5f, 1.0f));

            shader->setUniform("u_Model", model);
            shader->setUniform("u_Color", glm::vec4(ui.color, 1.0f));
            shader->setUniform("u_Luminance", ui.luminance);

            // --- BIND TEXTURE ---
            glActiveTexture(GL_TEXTURE0);

            auto tex = AssetManagerSystem::GetResource<Scene::Texture>(ui.textureid);
            // Handle missing texture 
            GLuint texID = (tex) ? tex->id : m_defaultWhiteTexture;
            glBindTexture(GL_TEXTURE_2D, texID);
            shader->setUniform("s_Texture", 0);

            // --- DRAW ---
            glDrawElements(GL_TRIANGLES, meshData->subMeshes[0].indexCount, GL_UNSIGNED_INT, 0);
        }

        shader->disuse();

        // 6. Restore Critical 3D States
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glDisable(GL_BLEND); // (Or keep enabled if your 3D engine uses it)
    }

    void GraphicsSystem::LoadAndCacheMesh(const std::string& meshName)
    {
        // Primitive meshes should be generated by PreGeneratePrimitives, not loaded from file
        if (meshName == "Quad" || meshName == "Plane" ||
            meshName == "Cube" || meshName == "Sphere" ||
            meshName == "SkyBox")
        {
            // Primitive meshes should be generated by PreGeneratePrimitives, not loaded from fileZZZ
            LOGE("Attempted to load primitive '%s' via file loader. Primitives should be pre-generated.", meshName.c_str());
            return;
        }

        try {
            LOGI("Loading mesh from file: %s", meshName.c_str());

            auto meshData = std::make_shared<Mesh::MeshData>(meshName);

            if (!meshData || meshData->vao == 0) {
                LOGE("Mesh '%s' loaded but VAO is 0!", meshName.c_str());
            }

            meshCache[meshName] = meshData;
        }
        catch (const std::exception& e) {
            LOGE("Failed to load mesh: %s. Error: %s", meshName.c_str(), e.what());
            meshCache[meshName] = nullptr;
        }
    }

    void GraphicsSystem::CalculateModelMatrix(const TransformComponent& transform, glm::mat4& modelMatrix)
    {

        // Translation
        glm::mat4 T = glm::translate(glm::mat4(1.0f), transform.position);

        // Rotation (Euler - Quaternion - Matrix)
        glm::vec3 rotationRadians = glm::radians(transform.rotation);
        glm::quat Q = glm::quat(rotationRadians);
        glm::mat4 R = glm::mat4_cast(Q);

        // Scale
        glm::mat4 S = glm::scale(glm::mat4(1.0f), transform.scale);

        // Combine: TRS
        modelMatrix = T * R * S;
    }

    void GraphicsSystem::LoadBlueNoiseTextures()
    {
        m_blue_noise_textures.resize(NUM_BLUE_NOISE_TEXTURES);
        glGenTextures(NUM_BLUE_NOISE_TEXTURES, m_blue_noise_textures.data());

        for (int i = 0; i < NUM_BLUE_NOISE_TEXTURES; i++)
        {
            std::string filename = "Extern/Graphics/Texture/blue_noise/LDR_LLL1_" + std::to_string(i) + ".png";
            int width, height, bytesPerPix;

            std::vector<unsigned char> texture_data = AssetManagerSystem::GetDataFromFile(std::filesystem::path(filename));
            unsigned char* data = stbi_load_from_memory(texture_data.data(), static_cast<int>(texture_data.size()), &width, &height, &bytesPerPix, 1);

            if (!data) {
                LOGE("Failed to load blue noise texture: %s", filename.c_str());
                continue;
            }

            glBindTexture(GL_TEXTURE_2D, m_blue_noise_textures[i]);
            glTexStorage2D(GL_TEXTURE_2D, 1, GL_R8, width, height);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RED, GL_UNSIGNED_BYTE, data);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

            stbi_image_free(data);
        }
    }

    void GraphicsSystem::PreGeneratePrimitives()
    {
        LOGI("Pre-generating primitive meshes...");
        meshCache["Cube"] = Mesh::generateCube();
        meshCache["Sphere"] = Mesh::generateSphere();
        meshCache["Quad"] = Mesh::generateQuad();
        meshCache["SkyBox"] = Mesh::generateSkyBox();
        LOGI("Primitive meshes cached.");
    }

    bool GraphicsSystem::isLightInFrustum(const glm::vec3& pos, float radius, const glm::mat4& viewProj)
    {
        // Extract planes (Note: In a real engine, you'd extract these ONCE per frame, not per object)
        auto planes = ExtractFrustumPlanes(viewProj);

        for (const auto& plane : planes) {
            // Distance from point to plane: dot(plane.xyz, pos) + plane.w
            float distance = glm::dot(glm::vec3(plane), pos) + plane.w;

            // If distance < -radius, the sphere is completely outside this plane
            if (distance < -radius) {
                return false;
            }
        }
        return true;
    }

    void GraphicsSystem::CalculateCascadeSplits(float nearPlane, float farPlane)
    {
        // Practical split scheme (PSSM) - blend between logarithmic and uniform
        float lambda = 0.75f;

        for (int i = 0; i < CASCADE_COUNT + 1; i++) {
            float p = static_cast<float>(i) / CASCADE_COUNT;

            float logSplit = nearPlane * std::pow(farPlane / nearPlane, p);
            float uniformSplit = nearPlane + (farPlane - nearPlane) * p;

            m_cascadeSplits[i] = lambda * logSplit + (1.0f - lambda) * uniformSplit;
        }
    }

    std::vector<glm::vec3> GraphicsSystem::GetFrustumCornersWorldSpace(
        const CameraComponent& camera,
        const glm::mat4& view,
        float nearZ,
        float farZ)
    {
        float aspect = (float)screen.x / screen.y;

        CameraComponent tempCam = camera;
        tempCam.nearPlane = nearZ;
        tempCam.farPlane = farZ;

        glm::mat4 sliceProj = Camera::getPerspective(tempCam, aspect);
        const glm::mat4 inv = glm::inverse(sliceProj * view);

        std::vector<glm::vec3> frustumCorners;
        frustumCorners.reserve(8);

        for (unsigned int x = 0; x < 2; ++x) {
            for (unsigned int y = 0; y < 2; ++y) {
                for (unsigned int z = 0; z < 2; ++z) {
                    glm::vec4 pt = inv * glm::vec4(
                        2.0f * x - 1.0f,
                        2.0f * y - 1.0f,
                        2.0f * z - 1.0f,
                        1.0f);
                    frustumCorners.push_back(glm::vec3(pt) / pt.w);
                }
            }
        }

        return frustumCorners;
    }

    std::array<glm::vec4, 6> GraphicsSystem::ExtractFrustumPlanes(const glm::mat4& viewProj)
    {
        std::array<glm::vec4, 6> planes;

        // Left
        planes[0] = glm::row(viewProj, 3) + glm::row(viewProj, 0);
        // Right
        planes[1] = glm::row(viewProj, 3) - glm::row(viewProj, 0);
        // Bottom
        planes[2] = glm::row(viewProj, 3) + glm::row(viewProj, 1);
        // Top
        planes[3] = glm::row(viewProj, 3) - glm::row(viewProj, 1);
        // Near
        planes[4] = glm::row(viewProj, 3) + glm::row(viewProj, 2);
        // Far
        planes[5] = glm::row(viewProj, 3) - glm::row(viewProj, 2);

        // Normalize planes
        for (int i = 0; i < 6; i++) {
            float length = glm::length(glm::vec3(planes[i]));
            planes[i] /= length;
        }

        return planes;
    }

    bool GraphicsSystem::IsBoxInFrustum(const float min[3], const float max[3], const glm::mat4& model, const glm::mat4& viewProj)
    {
        // 1. Convert raw arrays to GLM vectors
        glm::vec3 localMin = glm::vec3(min[0], min[1], min[2]);
        glm::vec3 localMax = glm::vec3(max[0], max[1], max[2]);

        // 2. Calculate Center and Extents (Half-Size)
        glm::vec3 localCenter = (localMin + localMax) * 0.5f;
        glm::vec3 localExtents = (localMax - localMin) * 0.5f;

        // 3. Transform Center to World Space
        glm::vec3 worldCenter = glm::vec3(model * glm::vec4(localCenter, 1.0f));

        // 4. Calculate World Radius (Safe approximation)
        // We scale the local extents by the model matrix to see how "big" the box is in world space.
        // The length of the sum of the scaled axes gives us the radius of a sphere that fully encloses the rotated box.
        glm::vec3 right = glm::vec3(model[0]) * localExtents.x;
        glm::vec3 up = glm::vec3(model[1]) * localExtents.y;
        glm::vec3 forward = glm::vec3(model[2]) * localExtents.z;

        // Radius of the circumscribed sphere of the OBB
        float worldRadius = glm::length(right + up + forward);

        // 5. Reuse your existing Frustum-Sphere check
        return isLightInFrustum(worldCenter, worldRadius, viewProj);
    }

    void GraphicsSystem::UploadBonePalettes(const std::vector<glm::mat4>& boneMatrices)
    {
        // Ensure UBO exists
        if (skinningUBO == 0) {
            glGenBuffers(1, &skinningUBO);
            glBindBuffer(GL_UNIFORM_BUFFER, skinningUBO);

            // Allocate maximum size once to avoid repeated reallocations
            glBufferData(GL_UNIFORM_BUFFER, maxBones * sizeof(glm::mat4), nullptr, GL_DYNAMIC_DRAW);
            glBindBuffer(GL_UNIFORM_BUFFER, 0);

            // Bind to a binding point that shaders use (example: binding = 0)
            const GLuint bindingPoint = 3u;
            glBindBufferBase(GL_UNIFORM_BUFFER, bindingPoint, skinningUBO);
        }

        size_t count = std::min<size_t>(boneMatrices.size(), static_cast<size_t>(maxBones));
        if (count == 0) return;

        glBindBuffer(GL_UNIFORM_BUFFER, skinningUBO);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, count * sizeof(glm::mat4), boneMatrices.data());
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

    bool GraphicsSystem::WorldToScreen(const glm::vec3& worldPos, glm::vec2& outScreenPos, float& outScale)
    {
        auto active_camera_entity = GetFirstActiveCamera();
        if (active_camera_entity == entt::null) return false;

        CameraComponent& camera = registry.get<CameraComponent>(active_camera_entity);

        // 1. Calculate Distance
        float dist = glm::distance(camera.position, worldPos);
        if (dist < 0.1f) dist = 0.1f;

        // 2. Calculate Scale
        // REFERENCE_DIST: The distance where the UI is exactly 1.0x its original size
        const float REFERENCE_DIST = 10.0f;

        // Big when Close, Small when Far
        outScale = REFERENCE_DIST / dist;

        // Allow it to get smaller (0.05) and much bigger (10.0)
        outScale = std::clamp(outScale, 0.05f, 10.0f);

        glm::mat4 view = Camera::getLookAt(camera);
        float aspect = (screen.y > 0) ? (float)screen.x / screen.y : 1.0f;
        glm::mat4 proj = Camera::getPerspective(camera, aspect);
        glm::mat4 viewProj = proj * view;

        glm::vec4 clipSpace = viewProj * glm::vec4(worldPos, 1.0f);

        if (clipSpace.w <= 0.0f) return false;

        glm::vec3 ndc = glm::vec3(clipSpace) / clipSpace.w;

        if (ndc.x < -1.2f || ndc.x > 1.2f || ndc.y < -1.2f || ndc.y > 1.2f) return false;

        outScreenPos.x = (ndc.x + 1.0f) * 0.5f * screen.x;
        outScreenPos.y = (ndc.y + 1.0f) * 0.5f * screen.y;

        return true;
    }

    // Event Handlers

    void GraphicsSystem::HANDLE_EVENT_LevelEditorToggled(Event_Level_Editor_Toggled& editor_toggled)
    {
        // Only draw the scene to the framebuffer if in level editor.
        m_isRenderingToFBO = editor_toggled.is_enabled;
    }

    void GraphicsSystem::HANDLE_EVENT_WindowResized(Event_Window_Resized& window_resize)
    {
        // Save current texture binding
        GLint prevTexture;
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &prevTexture);

        // Resize framebuffer and update screen size
        mainFBO.resize(window_resize.width, window_resize.height);
        screen.x = static_cast<float>(window_resize.width);
        screen.y = static_cast<float>(window_resize.height);

        // Restore previous texture binding
        glBindTexture(GL_TEXTURE_2D, prevTexture);
    }

} // namespace EngineFramework
