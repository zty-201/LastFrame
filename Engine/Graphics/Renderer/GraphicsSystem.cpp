/**
 * @file GraphicsSystem.cpp
 * @brief Implementation of the GraphicsSystem class for managing rendering operations.
 *
 * This file contains the implementation of the `GraphicsSystem` class, which is responsible
 * for initializing and managing the graphics pipeline in the EngineFramework. It handles
 * window creation, OpenGL context setup, rendering scenes, and managing framebuffers, shaders,
 * and other graphics-related resources.
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

#include "pch.hpp"
#include "global.hpp"
#include "Debug.h"
#include "GraphicsSystem.h"
#include "InputManager.hpp"
#include "AssetManager.hpp"
#include "MaterialComponent.h"

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

#if defined(DEBUG) && defined(PLATFORM_WINDOWS)
#include "System_LevelEditor.hpp"
class LevelEditor; //forward declaration
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

		//Listen for events
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

		// Configure GLFW for OpenGL ES 3.2 on desktop.
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
		FBO tempShadowFBO(FBO::CSMTextureArray, SHADOW_WIDTH, SHADOW_HEIGHT, &m_csmTextureArray, CASCADE_COUNT);
		m_shadowFBO = std::move(tempShadowFBO);

		// Create the point shadow map framebuffer
		FBO tempPointShadowFBO(FBO::Default, POINT_SHADOW_SIZE, POINT_SHADOW_SIZE);
		m_pointShadowFBO = std::move(tempPointShadowFBO);

		glGenFramebuffers(1, &m_pointShadowFBO.handle);
		glBindFramebuffer(GL_FRAMEBUFFER, m_pointShadowFBO.handle);

		// 3. Explicitly tell OpenGL this FBO has no color buffers
		GLenum drawBuffers[] = { GL_NONE };
		glDrawBuffers(1, drawBuffers);

		glReadBuffer(GL_NONE);

		// 4. Check status (It might still be "Incomplete Missing Attachment" until you bind a depth texture, 
		//    but this sets the correct "No Color" state for later).
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

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

			// Required for samplerCubeShadow to work!
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

		// Initialize instancing system
		//m_instancingSystem = std::make_unique<InstancingSystem>();
		//m_instancingSystem->Initialize(10000);

		// Create SSBO for point lights
		glGenBuffers(1, &m_pointLightSSBO);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_pointLightSSBO);
		glBufferData(GL_SHADER_STORAGE_BUFFER,
			sizeof(PointLightSSBO) * MAX_POINT_LIGHTS,
			nullptr, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

		// === CREATE DEFAULT TEXTURES ===
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

		glBindTexture(GL_TEXTURE_2D, 0);

		// === UBO SETUP ===
		glGenBuffers(1, &m_volumetricUBO);
		glBindBuffer(GL_UNIFORM_BUFFER, m_volumetricUBO);
		glBufferData(GL_UNIFORM_BUFFER, sizeof(VolumetricUBO), nullptr, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);

		// === VOLUMETRIC FOG SETUP (mostly unchanged) ===
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
		// Draw scene
		RenderScene();
	}

	void GraphicsSystem::PostUpdate(float deltaTime)
	{
		UNREFERENCED_PARAMETER(deltaTime);
#ifdef PLATFORM_WINDOWS
		// Swap buffers
		glfwSwapBuffers(pWindow);
#endif
	}

	void GraphicsSystem::RenderScene()
	{
		glEnable(GL_DEPTH_TEST);

		// Get camera information first
		auto active_camera_entity = GetFirstActiveCamera();
		if (active_camera_entity == entt::null) {
			LOGI("No cameras in registry yet, skipping render");
			return;
		}
		CameraComponent* active_camera = &registry.get<CameraComponent>(active_camera_entity);
		glm::mat4 V = Camera::getLookAt(*active_camera);
		float aspect = (screen.y > 0) ? (float)screen.x / screen.y : 1.0f;
		glm::mat4 P = Camera::getPerspective(*active_camera, aspect);

		// instancing temporary disabled, needs upgrade
		//m_instancingSystem->PrepareBatches();

		auto transform_view = registry.view<TransformComponent>();
		for (auto [entity, transform] : transform_view.each()) {
			CalculateModelMatrix(transform, transform.modelMatrix);
		}

		glm::mat4 viewProj = P * V;
		// Update point light SSBO
		UpdatePointLightSSBO(*active_camera, viewProj);
		// 1. Create shadow map
		ShadowPass();
		// 2. Update all uniforms now that shadow pass is done
		UpdateUBOs(*active_camera, V, P);
		// 3. Inject shadowed light into the volume
		VolumetricLightInjectionPass();
		// 4. Integrate the light through the volume
		VolumetricRayMarchPass();
		// 5. Render all meshes
		MeshPass();
		// 6. Render the skybox last
		//SkyboxPass();

		// 7. Copy to screen if needed
		if (!m_isRenderingToFBO) {
			glBindFramebuffer(GL_READ_FRAMEBUFFER, mainFBO.handle);
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
			glBlitFramebuffer(0, 0, (GLint)screen.x, (GLint)screen.y, 0, 0, (GLint)screen.x, (GLint)screen.y, GL_COLOR_BUFFER_BIT, GL_NEAREST);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}

		m_prev_view_projection = P * V;
		m_frame_idx++;
	}

	void GraphicsSystem::UpdatePointLightSSBO(const CameraComponent& camera, const glm::mat4& viewProj) {
		m_pointLightData.clear();
		std::vector<std::pair<int, float>> lightDistances;  // For shadow selection

		// 1. Collect ALL lights (visible or not for now)
		for (auto [entity, light] : registry.view<LightComponent>().each()) {
			if (light.type && m_pointLightData.size() < MAX_POINT_LIGHTS) {  // point light
				PointLightSSBO data;
				data.position_radius = glm::vec4(light.position, light.radius);
				data.color_intensity = glm::vec4(light.color * light.intensity, 0.0f);
				data.attenuation = glm::vec4(light.constant, light.linear, light.quadratic, 0.0f);
				data.shadow_index = glm::vec4(-1.0f, 0, 0, 0);  // Default: no shadow

				// Track distance for shadow assignment
				float dist = glm::length(camera.position - light.position);
				if (isLightInFrustum(light.position, light.radius, viewProj)) {
					lightDistances.push_back({ static_cast<int>(m_pointLightData.size()), dist });
				}

				m_pointLightData.push_back(data);
			}
		}

		// 2. Sort visible lights by distance and assign shadow indices
		std::sort(lightDistances.begin(), lightDistances.end(),
			[](const auto& a, const auto& b) { return a.second < b.second; });

		int shadowCount = std::min(MAX_SHADOW_CASTING_LIGHTS, (int)lightDistances.size());
		for (int i = 0; i < shadowCount; i++) {
			int lightIndex = lightDistances[i].first;
			m_pointLightData[lightIndex].shadow_index.x = float(i);  // This light uses cubemap slot i
		}

		// 3. Upload ALL lights (the full buffer)
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_pointLightSSBO);
		glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0,
			sizeof(PointLightSSBO) * m_pointLightData.size(),
			m_pointLightData.data());
	}

	void GraphicsSystem::RenderDirectionalShadow()
	{
		LightComponent* dirLight = nullptr;
		for (auto [entity, light] : registry.view<LightComponent>().each()) {
			if (!light.type) {  // false = directional
				dirLight = &light;
				break;
			}
		}

		if (!dirLight) return;

		// Get camera info
		auto active_camera_entity = GetFirstActiveCamera();
		if (active_camera_entity == entt::null) return;

		CameraComponent& camera = registry.get<CameraComponent>(active_camera_entity);
		glm::mat4 camView = Camera::getLookAt(camera);

		// Calculate cascade splits
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

		// Render each cascade
		for (int cascade = 0; cascade < CASCADE_COUNT; cascade++) {
			// Bind to the cascade layer
			glBindFramebuffer(GL_FRAMEBUFFER, m_shadowFBO.handle);
			glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
				m_csmTextureArray, 0, cascade);
			glClear(GL_DEPTH_BUFFER_BIT);

			// Calculate per-cascade bias
			float cascadeBias = m_shadow_bias * (1.0f + float(cascade) * 0.5f);
			m_cascadeBiases[cascade] = cascadeBias;

			// Calculate frustum corners for this cascade
			float nearSplit = m_cascadeSplits[cascade];
			float farSplit = m_cascadeSplits[cascade + 1];

			// Get frustum corners in world space
			std::vector<glm::vec3> frustumCorners = GetFrustumCornersWorldSpace(
				camera, camView, nearSplit, farSplit);

			// Calculate bounding box
			glm::vec3 center(0.0f);
			for (const auto& corner : frustumCorners) {
				center += corner;
			}
			center /= frustumCorners.size();

			// Create light view matrix looking at cascade center
			glm::vec3 lightDir = dirLight->direction;

			// Use a stable up vector
			glm::vec3 up = glm::abs(lightDir.y) > 0.999f ? glm::vec3(1, 0, 0) : glm::vec3(0, 1, 0);

			// 1. Create Light View Matrix anchored at the Origin (Rotation only)
			// This ensures the coordinate system axes never shift, only rotate with the light.
			glm::mat4 lightView = glm::lookAt(glm::vec3(0.0f), lightDir, up);

			// 2. Calculate Bounding Sphere of the Frustum
			// This is "Industry Standard" because a sphere is rotation-invariant.
			// It prevents the shadow map from shrinking/growing when you turn the camera.
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

			// 3. Transform Center to Light Space
			glm::vec3 lightSpaceCenter = glm::vec3(lightView * glm::vec4(frustumCenter, 1.0f));

			// 4. Calculate Texel Size based on Sphere Diameter
			// The diameter is constant per cascade, so texel size is constant -> No shimmering!
			float diameter = radius * 2.0f;
			float texelSize = diameter / SHADOW_WIDTH;

			// 5. Snap the Center to the Texel Grid
			lightSpaceCenter.x = floor(lightSpaceCenter.x / texelSize) * texelSize;
			lightSpaceCenter.y = floor(lightSpaceCenter.y / texelSize) * texelSize;

			// 6. Define Orthographic Bounds using the Sphere
			float minX = lightSpaceCenter.x - radius;
			float maxX = lightSpaceCenter.x + radius;
			float minY = lightSpaceCenter.y - radius;
			float maxY = lightSpaceCenter.y + radius;

			// Z-Bounds: We need to cover the frustum (center +/- radius) AND geometry between light and frustum.
			// We extend the 'near' plane (negative view Z) significantly towards the light.
			// In Light View space (looking down -Z), objects are at negative Z.
			float zMult = 10.0f;
			float minZ = lightSpaceCenter.z - radius * zMult; // Far back towards light
			float maxZ = lightSpaceCenter.z + radius * zMult; // Forward past frustum

			// GLM Ortho: left, right, bottom, top, zNear, zFar
			// For standard lookAt, zNear is distance to near plane (positive).
			// To capture range [minZ, maxZ], we use zNear = -maxZ, zFar = -minZ.
			glm::mat4 lightProj = glm::ortho(minX, maxX, minY, maxY, -maxZ, -minZ);

			m_cascadeViewProjMatrices[cascade] = lightProj * lightView;

			// Set shader uniforms 
			shader->setUniform("u_LightMatrix", m_cascadeViewProjMatrices[cascade]);

			// Render scene to this cascade
			auto view = registry.view<MeshComponent, TransformComponent>();
			for (auto [entity, meshComp, transformComp] : view.each()) {
				if (meshComp.meshName == "SkyBox") continue;

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

	void GraphicsSystem::RenderShadowCubemap(int cubemapSlot, const glm::vec4& lightPosRadius) {

		// TO implement: Use cubemapSlot to select the correct layer in a cubemap array
		UNREFERENCED_PARAMETER(cubemapSlot);

		glm::vec3 lightPos = glm::vec3(lightPosRadius);
		float radius = lightPosRadius.w;

		// Setup for point shadow rendering
		glViewport(0, 0, POINT_SHADOW_SIZE, POINT_SHADOW_SIZE);
		glBindFramebuffer(GL_FRAMEBUFFER, m_pointShadowFBO.handle);
		glEnable(GL_DEPTH_TEST);
		glCullFace(GL_FRONT);

		// Shadow projection matrix (90 degree FOV for cubemap)
		glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, radius);

		// 6 view matrices for cubemap faces
		glm::mat4 shadowViews[6] = {
			glm::lookAt(lightPos, lightPos + glm::vec3(1,0,0), glm::vec3(0,-1,0)),  // +X
			glm::lookAt(lightPos, lightPos + glm::vec3(-1,0,0), glm::vec3(0,-1,0)), // -X
			glm::lookAt(lightPos, lightPos + glm::vec3(0,1,0), glm::vec3(0,0,1)),   // +Y
			glm::lookAt(lightPos, lightPos + glm::vec3(0,-1,0), glm::vec3(0,0,-1)),  // -Y
			glm::lookAt(lightPos, lightPos + glm::vec3(0,0,1), glm::vec3(0,-1,0)),  // +Z
			glm::lookAt(lightPos, lightPos + glm::vec3(0,0,-1), glm::vec3(0,-1,0))   // -Z
		};

		// Get point shadow shader
		std::shared_ptr<ShaderProgram> shader{};
		if (point_shadow_shader_guid != xresource::full_guid{}) {
			shader = AssetManagerSystem::GetResource<ShaderProgram>(point_shadow_shader_guid);
		}
		else {
			using E_AM = AssetManagerSystem;
			point_shadow_shader_guid = E_AM::GetFullGuid(E_AM::SHADER_GUID, "PointShadow");
			shader = E_AM::GetResource<ShaderProgram>(point_shadow_shader_guid);
		}

		if (!shader) {
			LOGE("Point shadow shader not found!");
			return;
		}

		shader->use();
		shader->setUniform("u_LightPos", lightPos);
		shader->setUniform("u_FarPlane", radius);

		for (int face = 0; face < 6; face++) {
			// Attach the cube face to FBO
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
				GL_TEXTURE_CUBE_MAP_POSITIVE_X + face,
				m_pointShadowMapTextures[cubemapSlot], 0);

			glClear(GL_DEPTH_BUFFER_BIT);

			glm::mat4 lightMatrix = shadowProj * shadowViews[face];
			shader->setUniform("u_LightMatrix", lightMatrix);

			// Render all meshes from this light's perspective
			auto view = registry.view<MeshComponent, TransformComponent>();
			for (auto [entity, meshComp, transformComp] : view.each()) {
				if (meshComp.meshName == "SkyBox") continue;

				xresource::full_guid mesh_guid = AssetManagerSystem::GetFullGuid(
					AssetManagerSystem::MESH_GUID, meshComp.meshName);
				auto meshData = AssetManagerSystem::GetResource<Mesh::MeshData>(mesh_guid);

				if (!meshData || meshData->vao == 0) {
					meshData = meshCache[meshComp.meshName];
				}
				if (!meshData || meshData->vao == 0) continue;

				shader->setUniform("u_Model", transformComp.modelMatrix);

				glBindVertexArray(meshData->vao);
				for (const auto& subMesh : meshData->subMeshes) {
					glDrawElements(GL_TRIANGLES, subMesh.indexCount, GL_UNSIGNED_INT,
						(void*)(sizeof(unsigned int) * subMesh.baseIndex));
				}
			}
		}

		shader->disuse();

		// Restore state
		glCullFace(GL_BACK);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0, 0, (GLsizei)screen.x, (GLsizei)screen.y);
	}

	void GraphicsSystem::RenderPointLightShadows()
	{
		for (const auto& light : m_pointLightData) {
			if (light.shadow_index.x >= 0.0f) {
				int cubemapSlot = int(light.shadow_index.x);
				// Render to cubemap array at index cubemapSlot
				RenderShadowCubemap(cubemapSlot, light.position_radius);
			}
		}
	}

	void GraphicsSystem::ShadowPass()
	{

		// Render directional shadow first
		RenderDirectionalShadow();

		// Then render point light shadows
		RenderPointLightShadows();

		
	}

	

	void GraphicsSystem::UpdateUBOs(const CameraComponent& camera, const glm::mat4& V, const glm::mat4& P)
	{
		glBindBuffer(GL_UNIFORM_BUFFER, m_volumetricUBO);
		VolumetricUBO* ubo = (VolumetricUBO*)glMapBufferRange(GL_UNIFORM_BUFFER, 0, sizeof(VolumetricUBO), GL_MAP_WRITE_BIT);

		LightComponent* dirLight = nullptr;
		for (auto [entity, light] : registry.view<LightComponent>().each()) {
			if (!light.type) { // false = directional
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
		for (int i = 0; i < CASCADE_COUNT; i++) {
			ubo->cascade_view_proj[i] = m_cascadeViewProjMatrices[i];
		}
		ubo->cascade_splits = glm::vec4(m_cascadeSplits[1], m_cascadeSplits[2],m_cascadeSplits[3], m_cascadeSplits[4]);
		ubo->cascade_biases = glm::vec4(m_cascadeBiases[0], m_cascadeBiases[1], m_cascadeBiases[2], m_cascadeBiases[3]);
		ubo->light_direction = glm::vec4(direction, 0.0f);
		ubo->light_color = glm::vec4(color * intensity, m_ambient_light_intensity);
		ubo->camera_position = glm::vec4(camera.position, 1.0f);
		ubo->bias_near_far_pow = glm::vec4(m_shadow_bias, camera.nearPlane, camera.farPlane, m_depth_power);
		ubo->aniso_density_scattering_absorption = glm::vec4(m_anisotropy, m_density, m_scattering, m_absorption);
		ubo->time = glm::vec4(static_cast<float>(get_native_time()), 0, 0, 0);
		ubo->width_height = glm::ivec4(screen.x, screen.y, 0, 0);

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

		glBindImageTexture(0, m_temporal_integration_voxel_grid[write_idx], 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA16F);

		glActiveTexture(GL_TEXTURE0 + TextureUnits::VOLUMETRIC_HISTORY_UNIT);
		glBindTexture(GL_TEXTURE_3D, m_temporal_integration_voxel_grid[read_idx]);
		shader->setUniform("s_History", TextureUnits::VOLUMETRIC_HISTORY_UNIT);

		glActiveTexture(GL_TEXTURE0 + TextureUnits::VOLUMETRIC_BLUE_NOISE_UNIT);
		glBindTexture(GL_TEXTURE_2D, m_blue_noise_textures[m_temporal_accumulation ? m_frame_idx % NUM_BLUE_NOISE_TEXTURES : 0]);
		shader->setUniform("s_BlueNoise", TextureUnits::VOLUMETRIC_BLUE_NOISE_UNIT);

		glActiveTexture(GL_TEXTURE0 + TextureUnits::SHADOW_MAP_UNIT);
		glBindTexture(GL_TEXTURE_2D_ARRAY, m_csmTextureArray);  
		shader->setUniform("s_CascadeShadowMaps", TextureUnits::SHADOW_MAP_UNIT);

		int baseUnit = TextureUnits::POINT_SHADOW_MAP_UNIT;

		for (int i = 0; i < MAX_SHADOW_CASTING_LIGHTS; i++) {
			glActiveTexture(GL_TEXTURE0 + baseUnit + i);
			glBindTexture(GL_TEXTURE_CUBE_MAP, m_pointShadowMapTextures[i]);

			// Set the sampler uniform array element
			std::string uniformName = "s_PointShadowCubemaps[" + std::to_string(i) + "]";
			shader->setUniform(uniformName.c_str(), baseUnit + i);
		}

		shader->setUniform("u_NumPointLights", (int)m_pointLightData.size());
		shader->setUniform("u_Accumulation", m_frame_idx > 0 && m_temporal_accumulation);

		glDispatchCompute((VOXEL_GRID_SIZE_X + 7) / 8, (VOXEL_GRID_SIZE_Y + 7) / 8, (VOXEL_GRID_SIZE_Z + 0) / 1);
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

		glDispatchCompute((VOXEL_GRID_SIZE_X + 7) / 8, (VOXEL_GRID_SIZE_Y + 7) / 8, 1);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		shader->disuse();
	}

	void GraphicsSystem::MeshPass()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, mainFBO.handle);
		glViewport(0, 0, (GLsizei)screen.x, (GLsizei)screen.y);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clear for the first pass writing to mainFBO

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

		shader->use();
		glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_volumetricUBO);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_pointLightSSBO);

		// Bind common textures
		glActiveTexture(GL_TEXTURE0 + TextureUnits::SHADOW_MAP_UNIT);
		glBindTexture(GL_TEXTURE_2D_ARRAY, m_csmTextureArray);
		shader->setUniform("s_CascadeShadowMaps", TextureUnits::SHADOW_MAP_UNIT);

		int baseUnit = TextureUnits::POINT_SHADOW_MAP_UNIT;

		for (int i = 0; i < MAX_SHADOW_CASTING_LIGHTS; i++) {
			glActiveTexture(GL_TEXTURE0 + baseUnit + i);
			glBindTexture(GL_TEXTURE_CUBE_MAP, m_pointShadowMapTextures[i]);

			// Set the sampler uniform array element
			std::string uniformName = "s_PointShadowCubemaps[" + std::to_string(i) + "]";
			shader->setUniform(uniformName.c_str(), baseUnit + i);
		}

		glActiveTexture(GL_TEXTURE0 + TextureUnits::VOLUMETRIC_VOXEL_GRID_UNIT);
		glBindTexture(GL_TEXTURE_3D, m_ray_march_voxel_grid);
		shader->setUniform("s_VoxelGrid", TextureUnits::VOLUMETRIC_VOXEL_GRID_UNIT);
		shader->setUniform("u_Tricubic", m_tricubic_filtering);
		// Pass the number of point lights
		shader->setUniform("u_NumPointLights", (int)m_pointLightData.size());
		if (SceneManager::GetInstance().is_file_loaded) {
			// Add keyboard control (or use ImGui if you have it)
			if (ImGui::IsKeyDown(ImGuiKey_F1)) debugMode = 0;  // Normal
			if (ImGui::IsKeyDown(ImGuiKey_F2)) debugMode = 1;  // Cascade colors
			if (ImGui::IsKeyDown(ImGuiKey_F3)) debugMode = 2;  // Shadow UV
			if (ImGui::IsKeyDown(ImGuiKey_F4)) debugMode = 3;  // Raw shadow
			if (ImGui::IsKeyDown(ImGuiKey_F5)) debugMode = 4;  // Depth values
			if (ImGui::IsKeyDown(ImGuiKey_F6)) debugMode = 5;  // Cascade gradients
			if (ImGui::IsKeyDown(ImGuiKey_F7)) debugMode = 6;  // Cascade 0 only
			if (ImGui::IsKeyDown(ImGuiKey_F8)) debugMode = 7;  // Light direction
			if (ImGui::IsKeyDown(ImGuiKey_F9)) debugMode = 8;  // Distance
			if (ImGui::IsKeyDown(ImGuiKey_F10)) debugMode = 9; // Cascade 0 lit/shadow
		}
		

		shader->setUniform("u_DebugMode", debugMode);

		// Log current mode
		static int lastDebugMode = -1;
		if (debugMode != lastDebugMode) {
			const char* modeNames[] = {
				"Normal", "Cascade Colors", "Shadow UV", "Raw Shadow",
				"Depth Values", "Cascade Gradients", "Cascade 0 Only",
				"Light Direction", "Distance", "Cascade 0 Lit/Shadow"
			};
			LOGI("Debug Mode: %d - %s", debugMode, modeNames[debugMode]);
			lastDebugMode = debugMode;
		}

		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
		glDepthFunc(GL_LESS);

		auto view = registry.view<MeshComponent, TransformComponent>();
		for (auto [entity, meshComp, transformComp] : view.each())
		{
			if (meshComp.meshName == "SkyBox") continue;

			// 1. Get the GUID for the mesh by its name.
			xresource::full_guid mesh_guid = AssetManagerSystem::GetFullGuid(AssetManagerSystem::MESH_GUID, meshComp.meshName);

			// 2. Get the mesh resource using the GUID.
			auto meshData = AssetManagerSystem::GetResource<Mesh::MeshData>(mesh_guid);

			// If mesh is not found look for it in the cache
			if (!meshData || meshData->vao == 0) {
				meshData = meshCache[meshComp.meshName];
			} 

			// If still not found, skip rendering this mesh
			if (!meshData || meshData->vao == 0) continue;

			// Set the per-object model matrix
			shader->setUniform("u_Model", transformComp.modelMatrix);

			glBindVertexArray(meshData->vao);

			const MaterialComponent* override_material = registry.try_get<MaterialComponent>(entity);

			// Loop through each sub-mesh of this model
			for (const auto& subMesh : meshData->subMeshes)
			{
				// Get the material for this specific sub-mesh
				if (subMesh.materialIndex < 0 || subMesh.materialIndex >= meshData->materials.size()) continue;
				const MaterialComponent& material = override_material ? *override_material : meshData->materials[subMesh.materialIndex];

				// --- SET MATERIAL FACTOR UNIFORMS ---
				shader->setUniform("u_AlbedoFactor", material.albedoFactor);
				shader->setUniform("u_RoughnessFactor", material.roughnessFactor);
				shader->setUniform("u_MetallicFactor", material.metallicFactor);

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
				glBindTexture(GL_TEXTURE_2D, (metalTex && metalTex->id != 0) ? metalTex->id : m_defaultWhiteTexture);
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

				// Draw ONLY this part of the mesh
				glDrawElements(GL_TRIANGLES, subMesh.indexCount, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * subMesh.baseIndex));
			}
		}

		shader->disuse();
	}

	void GraphicsSystem::SkyboxPass()
	{
		// The main framebuffer is already bound from the Mesh pass. We just draw over it.
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

		glDepthFunc(GL_LEQUAL); // Standard skybox trick

		auto skybox_view = registry.view<MeshComponent, SkyboxRenderPass>();
		for (auto [entity, meshComp] : skybox_view.each()) {
			if (meshCache.find(meshComp.meshName) == meshCache.end()) LoadAndCacheMesh(meshComp.meshName);

			auto& meshData = meshCache[meshComp.meshName];
			if (!meshData || meshData->vao == 0) continue;

			glBindVertexArray(meshData->vao);
			// The Skybox should just be one submesh
			if (!meshData->subMeshes.empty()) {
				glDrawElements(GL_TRIANGLES, meshData->subMeshes[0].indexCount, GL_UNSIGNED_INT, 0);
			}
			else {
				LOGE("Skybox mesh '%s' has no submeshes!", meshComp.meshName.c_str());
			}
		}

		glDepthFunc(GL_LESS);
		shader->disuse();
	}
	
	void GraphicsSystem::LoadAndCacheMesh(const std::string& meshName)
	{
		if (meshName == "Quad" || meshName == "Plane" || meshName == "Cube" || meshName == "Sphere" || meshName == "SkyBox")
		{
			// primitives meshes doesnt seem to be read by level editor side, so not really in use now
			LOGE("Attempted to load primitive '%s' via file loader. Primitives should be pre-generated.", meshName.c_str());
			return;
		}

		try {
			LOGI("Loading mesh from file: %s", meshName.c_str());
			auto meshData = std::make_shared<Mesh::MeshData>(meshName);
			meshCache[meshName] = meshData;
		}
		catch (const std::exception& e) {
			LOGE("Failed to load mesh: %s. Error: %s", meshName.c_str(), e.what());
			// Insert a null pointer to prevent trying to load it again.
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

	void GraphicsSystem::PreGeneratePrimitives() {
		LOGI("Pre-generating primitive meshes...");
		meshCache["Cube"] = Mesh::generateCube();
		meshCache["Sphere"] = Mesh::generateSphere();
		meshCache["Quad"] = Mesh::generateQuad();
		meshCache["SkyBox"] = Mesh::generateSkyBox();
		LOGI("Primitive meshes cached.");
	}
	
	bool GraphicsSystem::isLightInFrustum(const glm::vec3& lightPos, float radius, const glm::mat4& viewProj) {
		// Transform light center to clip space
		glm::vec4 clipPos = viewProj * glm::vec4(lightPos, 1.0f);

		// Early reject if behind camera
		if (clipPos.w < 0.0f) return false;

		// Perspective divide
		glm::vec3 ndc = glm::vec3(clipPos) / clipPos.w;

		// Check if sphere intersects NDC cube (expanded by radius)
		float radiusNDC = radius / clipPos.w; // Approximate radius in NDC

		return (std::abs(ndc.x) <= 1.0f + radiusNDC) &&
			(std::abs(ndc.y) <= 1.0f + radiusNDC) &&
			(ndc.z >= -1.0f - radiusNDC && ndc.z <= 1.0f + radiusNDC);
	}

	void GraphicsSystem::CalculateCascadeSplits(float nearPlane, float farPlane)
	{
		// Practical split scheme (PSSM) - blend between logarithmic and uniform
		float lambda = 0.75f;  // Weight towards logarithmic (0.95 = mostly log, good for outdoor scenes)

		for (int i = 0; i < CASCADE_COUNT + 1; i++) {
			float p = static_cast<float>(i) / CASCADE_COUNT;

			// Logarithmic split
			float log = nearPlane * std::pow(farPlane / nearPlane, p);

			// Uniform split
			float uniform = nearPlane + (farPlane - nearPlane) * p;

			// Blend
			m_cascadeSplits[i] = lambda * log + (1.0f - lambda) * uniform;
		}

		// Manual tweaks for better distribution (optional)
		// m_cascadeSplits = {nearPlane, 10.0f, 50.0f, 200.0f, farPlane};
	}

	std::vector<glm::vec3> GraphicsSystem::GetFrustumCornersWorldSpace(const CameraComponent& camera, const glm::mat4& view, float nearZ, float farZ)
	{

		float aspect = (float)screen.x / screen.y;

		// Create a temporary camera with the cascade's near/far planes
		CameraComponent tempCam = camera;
		tempCam.nearPlane = nearZ;
		tempCam.farPlane = farZ;

		// Generate projection matrix for this slice
		glm::mat4 sliceProj = Camera::getPerspective(tempCam, aspect);
		const glm::mat4 inv = glm::inverse(sliceProj * view);

		// Generate the 8 corners
		std::vector<glm::vec3> frustumCorners;
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
	//==Event Handlers==//

	//When level editor is toggled on/off.
	void GraphicsSystem::HANDLE_EVENT_LevelEditorToggled(Event_Level_Editor_Toggled& editor_toggled)
	{
		//Only draw the scene to the framebuffer if in level editor.
		m_isRenderingToFBO = editor_toggled.is_enabled;
	}

	//When drawing area should be resized.
	void GraphicsSystem::HANDLE_EVENT_WindowResized(Event_Window_Resized& window_resize)
	{
		// Save current texture bindings
		GLint prevTexture;
		glGetIntegerv(GL_TEXTURE_BINDING_2D, &prevTexture);

		// Resize framebuffers
		mainFBO.resize(window_resize.width, window_resize.height);
		screen.x = static_cast<float>(window_resize.width);
		screen.y = static_cast<float>(window_resize.height);

		// Restore previous texture bindings
		glBindTexture(GL_TEXTURE_2D, prevTexture);
	}

}
