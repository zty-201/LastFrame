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

#include "stb_image.h"
#define VOXEL_GRID_SIZE_X 160
#define VOXEL_GRID_SIZE_Y 90
#define VOXEL_GRID_SIZE_Z 128
#define NUM_BLUE_NOISE_TEXTURES 16
#define SHADOW_WIDTH 4096
#define SHADOW_HEIGHT 4096

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
		FBO tempShadowFBO(FBO::TextureDepth, SHADOW_WIDTH, SHADOW_HEIGHT, &m_shadowMapTextureID);
		m_shadowFBO = std::move(tempShadowFBO);

		// Get shader GUIDs 
		mesh_shader_guid = AssetMgr::GetFullGuid(AssetMgr::SHADER_GUID, "Mesh");
		skybox_shader_guid = AssetMgr::GetFullGuid(AssetMgr::SHADER_GUID, "Skybox");
		shadow_shader_guid = AssetMgr::GetFullGuid(AssetMgr::SHADER_GUID, "ShadowMap");
		light_injection_shader_guid = AssetMgr::GetFullGuid(AssetMgr::SHADER_GUID, "LightInjection");
		ray_march_shader_guid = AssetMgr::GetFullGuid(AssetMgr::SHADER_GUID, "RayMarch");

		// Initialize instancing system
		//m_instancingSystem = std::make_unique<InstancingSystem>();
		//m_instancingSystem->Initialize(10000);

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

	void GraphicsSystem::ShadowPass()
	{
		auto light_view = registry.view<LightComponent>();
		if (light_view.empty()) return;
		auto& main_light = light_view.get<LightComponent>(light_view.front());

		float orthoSize = 1000.0f;  // Increased from 100
		glm::mat4 lightProjection = glm::ortho(-orthoSize, orthoSize,
			-orthoSize, orthoSize,
			-500.0f, 1000.0f);
		
		glm::vec3 lightPos = main_light.position;
		glm::vec3 lightTarget = glm::vec3(0.0f, 0.0f, 0.0f);
		glm::mat4 lightView = glm::lookAt(lightPos, lightTarget, glm::vec3(0.0, 1.0, 0.0));

		m_lightSpaceMatrix = lightProjection * lightView;

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

		glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_volumetricUBO);

		glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
		glBindFramebuffer(GL_FRAMEBUFFER, m_shadowFBO.handle);
		glClear(GL_DEPTH_BUFFER_BIT);
		glEnable(GL_CULL_FACE);
		glCullFace(GL_FRONT);

		auto view = registry.view<MeshComponent, TransformComponent>();
		for (auto [entity, meshComp, transformComp] : view.each())
		{
			if (meshComp.meshName == "SkyBox") continue;

			// 1. Get the GUID for the mesh by its name.
			xresource::full_guid mesh_guid = AssetManagerSystem::GetFullGuid(AssetManagerSystem::MESH_GUID, meshComp.meshName);

			// 2. Get the mesh resource using the GUID.
			auto meshData = AssetManagerSystem::GetResource<Mesh::MeshData>(mesh_guid);
			if (!meshData || meshData->vao == 0) continue;

			// For shadows, we don't care about materials, we just draw all geometry.
			shader->setUniform("u_Model", transformComp.modelMatrix);

			glBindVertexArray(meshData->vao);
			// Draw all sub-meshes
			for (const auto& subMesh : meshData->subMeshes) {
				glDrawElements(GL_TRIANGLES, subMesh.indexCount, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * subMesh.baseIndex));
			}
		}

		glCullFace(GL_BACK);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		shader->disuse();
		glViewport(0, 0, (GLsizei)screen.x, (GLsizei)screen.y);
	}

	void GraphicsSystem::UpdateUBOs(const CameraComponent& camera, const glm::mat4& V, const glm::mat4& P)
	{
		glBindBuffer(GL_UNIFORM_BUFFER, m_volumetricUBO);
		VolumetricUBO* ubo = (VolumetricUBO*)glMapBufferRange(GL_UNIFORM_BUFFER, 0, sizeof(VolumetricUBO), GL_MAP_WRITE_BIT);

		auto light_view = registry.view<LightComponent>();
		LightComponent& main_light = light_view.get<LightComponent>(light_view.front());
		glm::vec3 lightDir = glm::normalize(glm::vec3(0.0f) - main_light.position);

		ubo->view = V;
		ubo->projection = P;
		ubo->view_proj = P * V;
		ubo->prev_view_proj = m_frame_idx == 0 ? P * V : m_prev_view_projection;
		ubo->light_view_proj = m_lightSpaceMatrix;
		ubo->inv_view_proj = glm::inverse(ubo->view_proj);
		ubo->light_direction = glm::vec4(lightDir, 0.0f);
		ubo->light_color = glm::vec4(main_light.color * main_light.intensity, m_ambient_light_intensity);
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
		glBindTexture(GL_TEXTURE_2D, m_shadowMapTextureID);
		shader->setUniform("s_ShadowMap", TextureUnits::SHADOW_MAP_UNIT);

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

		// Bind common textures
		glActiveTexture(GL_TEXTURE0 + TextureUnits::SHADOW_MAP_UNIT);
		glBindTexture(GL_TEXTURE_2D, m_shadowMapTextureID);
		shader->setUniform("s_ShadowMap", TextureUnits::SHADOW_MAP_UNIT);

		glActiveTexture(GL_TEXTURE0 + TextureUnits::VOLUMETRIC_VOXEL_GRID_UNIT);
		glBindTexture(GL_TEXTURE_3D, m_ray_march_voxel_grid);
		shader->setUniform("s_VoxelGrid", TextureUnits::VOLUMETRIC_VOXEL_GRID_UNIT);
		shader->setUniform("u_Tricubic", m_tricubic_filtering);

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

			// Loop through each sub-mesh of this model
			for (const auto& subMesh : meshData->subMeshes)
			{
				// Get the material for this specific sub-mesh
				if (subMesh.materialIndex < 0 || subMesh.materialIndex >= meshData->materials.size()) continue;
				const auto& material = meshData->materials[subMesh.materialIndex];

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