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


#if defined(DEBUG) && defined(PLATFORM_WINDOWS)
#include "System_LevelEditor.hpp"
class LevelEditor; //forward declaration
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

		// Create the G-buffer framebuffer for deferred rendering
		gBufferFBO = FBO(FBO::Deferred, WIDTH, HEIGHT);

		// Get shader GUIDs
		forward_shader_guid = AssetMgr::GetFullGuid(AssetMgr::SHADER_GUID, "Forward");
		geometric_shader_guid = AssetMgr::GetFullGuid(AssetMgr::SHADER_GUID, "Geometric");
		lighting_shader_guid = AssetMgr::GetFullGuid(AssetMgr::SHADER_GUID, "Lighting");

		// Initialize instancing system
		m_instancingSystem = std::make_unique<InstancingSystem>();
		m_instancingSystem->Initialize(10000);

		// Initialize the fullscreen quad for the lighting pass
		InitializeFullscreenQuad();

		// Other setup code...
		pScene = std::make_unique<Scene>();
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

	void GraphicsSystem::GeometryPass(glm::mat4& V, glm::mat4& P)
	{
		//-------------------------------------------------------------------------
		// GEOMETRY PASS - Render scene geometry info to G-buffer
		//-------------------------------------------------------------------------
		glBindFramebuffer(GL_FRAMEBUFFER, gBufferFBO.handle);
		glViewport(0, 0, static_cast<GLsizei>(screen.x), static_cast<GLsizei>(screen.y));
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		std::shared_ptr<ShaderProgram> geometric_shader{};
		if (geometric_shader_guid != xresource::full_guid{}) {
			geometric_shader = EngineFramework::AssetManagerSystem::GetResource<ShaderProgram>(geometric_shader_guid);
		}
		else {
			using E_AM = EngineFramework::AssetManagerSystem;
			geometric_shader_guid = E_AM::GetFullGuid(E_AM::SHADER_GUID, "Geometric");
			geometric_shader = E_AM::GetResource<ShaderProgram>(geometric_shader_guid);
		}

		geometric_shader->use();
		geometric_shader->setUniform("V", V);
		geometric_shader->setUniform("P", P);

		glDepthFunc(GL_LESS);
		glDepthMask(GL_TRUE);
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);

		// ====================================================================
		// INSTANCED RENDERING - ITERATE THROUGH BATCHES
		// ====================================================================

		const auto& deferredBatches = m_instancingSystem->GetDeferredBatches();

		for (const auto& [batchKey, batch] : deferredBatches) {
			// Load mesh if not cached
			if (meshCache.find(batch.meshName) == meshCache.end()) {
				LoadAndCacheMesh(batch.meshName);
			}

			auto& [vao, indexCount] = meshCache[batch.meshName];
			glBindVertexArray(vao);

			// Bind instance SSBO to binding point 0
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, batch.instanceSSBO);

			geometric_shader->setUniform("material.Ka", batch.batchMaterial.Ka);
			geometric_shader->setUniform("material.Kd", batch.batchMaterial.Kd);
			geometric_shader->setUniform("material.Ks", batch.batchMaterial.Ks);
			geometric_shader->setUniform("material.rough", batch.batchMaterial.rough);
			geometric_shader->setUniform("material.metal", batch.batchMaterial.metal);
			geometric_shader->setUniform("material.shininess", batch.batchMaterial.shininess);
			geometric_shader->setUniform("material.effect", batch.batchMaterial.effect);

			// Bind diffuse texture for this batch
			bool hasTexture = false;
			if (!batchKey.textureGUID.empty()) {
				auto texture = AssetManagerSystem::GetResource<Scene::Texture>(batchKey.textureGUID);
				if (texture && texture->id != 0) {
					glActiveTexture(GL_TEXTURE0);
					glBindTexture(texture->target, texture->id);
					geometric_shader->setUniform("DiffuseTex", 0);
					hasTexture = true;
				}
			}
			geometric_shader->setUniform("UseTexture", hasTexture);

			// Bind normal map for this batch
			bool hasNormalMap = false;
			if (!batchKey.normalMapGUID.empty()) {
				auto normalMap = AssetManagerSystem::GetResource<Scene::Texture>(batchKey.normalMapGUID);
				if (normalMap && normalMap->id != 0) {
					glActiveTexture(GL_TEXTURE2);
					glBindTexture(GL_TEXTURE_2D, normalMap->id);
					geometric_shader->setUniform("NormalMap", 2);
					hasNormalMap = true;
				}
			}
			geometric_shader->setUniform("UseNormalMap", hasNormalMap);

			// Single draw call for all instances in this batch
			glDrawElementsInstanced(GL_TRIANGLES,
				static_cast<GLsizei>(indexCount),
				GL_UNSIGNED_INT,
				nullptr,
				static_cast<GLsizei>(batch.currentInstanceCount));
		}

		glBindVertexArray(0);
		geometric_shader->disuse();
	}

	void GraphicsSystem::LightingPass(glm::mat4& V)
	{
		//-------------------------------------------------------------------------
		// LIGHTING PASS - Calculate lighting using G-buffer data
		//-------------------------------------------------------------------------
		auto light_view = registry.view<LightComponent>();

		// render to mainFBO
		glBindFramebuffer(GL_FRAMEBUFFER, mainFBO.handle);
		glViewport(0, 0, static_cast<GLsizei>(screen.x), static_cast<GLsizei>(screen.y));
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		std::shared_ptr<ShaderProgram> lighting_shader{};
		if (lighting_shader_guid != xresource::full_guid{})
		{
			lighting_shader = EngineFramework::AssetManagerSystem::GetResource<ShaderProgram>(lighting_shader_guid);
		}
		else
		{
			using E_AM = EngineFramework::AssetManagerSystem;
			lighting_shader_guid = E_AM::GetFullGuid(E_AM::SHADER_GUID, "Lighting");
			lighting_shader = E_AM::GetResource<ShaderProgram>(lighting_shader_guid);
		}

		lighting_shader->use();

		// Bind G-buffer textures
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, gBufferFBO.getPositionTexture());
		lighting_shader->setUniform("gPosition", 0);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, gBufferFBO.getNormalTexture());
		lighting_shader->setUniform("gNormal", 1);

		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, gBufferFBO.getAlbedoSpecTexture());
		lighting_shader->setUniform("gAlbedoSpec", 2);

		// Set lighting uniforms
		lighting_shader->setUniform("V", V);

		// Set light uniforms
		int light_index = 0;
		for (auto entity : light_view) {
			auto& light = light_view.get<LightComponent>(entity);

			std::stringstream s;
			s << "light[" << light_index << "].position";
			lighting_shader->setUniform(s.str(), light.position);

			if (light.L.size() > 0) {
				s.str("");
				s << "light[" << light_index << "].L";
				lighting_shader->setUniform(s.str(), light.L[0]);

				s.str("");
				s << "light[" << light_index << "].La";
				lighting_shader->setUniform(s.str(), light.L[0]);
			}

			if (light.L.size() > 1) {
				s.str("");
				s << "light[" << light_index << "].Ld";
				lighting_shader->setUniform(s.str(), light.L[1]);
			}

			if (light.L.size() > 2) {
				s.str("");
				s << "light[" << light_index << "].Ls";
				lighting_shader->setUniform(s.str(), light.L[2]);
			}

			light_index++;
		}

		// Material defaults for lighting calculations
		lighting_shader->setUniform("material.Ka", glm::vec3(0.1f));
		lighting_shader->setUniform("material.Kd", glm::vec3(0.7f));
		lighting_shader->setUniform("material.Ks", glm::vec3(1.0f));
		lighting_shader->setUniform("material.shininess", 32.0f);
		lighting_shader->setUniform("material.rough", 0.5f);
		lighting_shader->setUniform("material.metal", 0.0f);

		// ========== FOG UNIFORMS ==========
		lighting_shader->setUniform("fogColor", glm::vec3(0.5f, 0.55f, 0.65f));  // Darker fog
		lighting_shader->setUniform("fogDensity", 0.20f);  // INCREASE from 0.08f
		lighting_shader->setUniform("fogAbsorption", 0.7f);  // INCREASE from 0.5f
		lighting_shader->setUniform("fogLightDirection", glm::normalize(glm::vec3(0.2f, 0.8f, 0.3f)));
		lighting_shader->setUniform("fogLightColor", glm::vec3(1.0f, 0.95f, 0.85f));
		lighting_shader->setUniform("fogLightIntensity", 1.5f);  // INCREASE from 1.2f
		lighting_shader->setUniform("fogAnisotropy", 0.4f);  // INCREASE from 0.3f

		// Draw fullscreen quad
		glDisable(GL_DEPTH_TEST);
		glBindVertexArray(fullscreenQuadVAO);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		glEnable(GL_DEPTH_TEST);

		lighting_shader->disuse();

		// Copy depth buffer from G-buffer to mainFBO for forward rendering
		glBindFramebuffer(GL_READ_FRAMEBUFFER, gBufferFBO.handle);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, mainFBO.handle);
		glBlitFramebuffer(
			0, 0, static_cast<GLuint>(screen.x), static_cast<GLuint>(screen.y),
			0, 0, static_cast<GLuint>(screen.x), static_cast<GLuint>(screen.y),
			GL_DEPTH_BUFFER_BIT, GL_NEAREST
		);
		glBindFramebuffer(GL_FRAMEBUFFER, mainFBO.handle);
	}

	void GraphicsSystem::ForwardRenderPass(glm::mat4& V, glm::mat4& P)
	{
		//-------------------------------------------------------------------------
		// FORWARD PASS - Render transparent objects and skybox
		//-------------------------------------------------------------------------
		glBindFramebuffer(GL_FRAMEBUFFER, mainFBO.handle);
		glViewport(0, 0, static_cast<GLsizei>(screen.x), static_cast<GLsizei>(screen.y));
		// Don't clear - we're compositing on top of lighting result

		auto light_view = registry.view<LightComponent>();

		std::shared_ptr<ShaderProgram> forward_shader{};
		if (forward_shader_guid != xresource::full_guid{})
		{
			forward_shader = EngineFramework::AssetManagerSystem::GetResource<ShaderProgram>(forward_shader_guid);
		}
		else
		{
			using E_AM = EngineFramework::AssetManagerSystem;
			forward_shader_guid = E_AM::GetFullGuid(E_AM::SHADER_GUID, "Forward");
			forward_shader = E_AM::GetResource<ShaderProgram>(forward_shader_guid);
		}

		forward_shader->use();

		// Set global uniforms
		forward_shader->setUniform("Pass", 0);
		forward_shader->setUniform("V", V);
		forward_shader->setUniform("P", P);

		// Set light uniforms
		int light_index = 0;
		for (auto entity : light_view) {
			auto& light = light_view.get<LightComponent>(entity);
			Light::setUniforms(light, forward_shader, light_index++);
		}

		// --- RENDER SCENE OBJECTS ---
		glDepthFunc(GL_LESS);
		glDepthMask(GL_TRUE);
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);

		const auto& forwardBatches = m_instancingSystem->GetForwardBatches();

		for (const auto& [batchKey, batch] : forwardBatches) {
			if (batchKey.meshName == "SkyBox") continue;

			bool isCheckerboard = (batch.batchMaterial.effect == 3.0f);
			if (isCheckerboard) {
				glCullFace(GL_FRONT);
			}
			if (meshCache.find(batch.meshName) == meshCache.end()) {
				LoadAndCacheMesh(batch.meshName);
			}

			auto& [vao, indexCount] = meshCache[batch.meshName];
			glBindVertexArray(vao);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, batch.instanceSSBO);

			// Set per-batch material uniforms
			forward_shader->setUniform("material.Ka", batch.batchMaterial.Ka);
			forward_shader->setUniform("material.Kd", batch.batchMaterial.Kd);
			forward_shader->setUniform("material.Ks", batch.batchMaterial.Ks);
			forward_shader->setUniform("material.rough", batch.batchMaterial.rough);
			forward_shader->setUniform("material.metal", batch.batchMaterial.metal);
			forward_shader->setUniform("material.shininess", batch.batchMaterial.shininess);
			forward_shader->setUniform("material.effect", batch.batchMaterial.effect);

			// Bind textures
			bool hasTexture = false;
			if (!batchKey.textureGUID.empty()) {
				auto texture = AssetManagerSystem::GetResource<Scene::Texture>(batchKey.textureGUID);
				if (texture && texture->id != 0) {
					glActiveTexture(GL_TEXTURE0);
					glBindTexture(texture->target, texture->id);
					forward_shader->setUniform("DiffuseTex", 0);
					hasTexture = true;
				}
			}
			forward_shader->setUniform("UseTexture", hasTexture);

			bool hasNormalMap = false;
			if (!batchKey.normalMapGUID.empty()) {
				auto normalMap = AssetManagerSystem::GetResource<Scene::Texture>(batchKey.normalMapGUID);
				if (normalMap && normalMap->id != 0) {
					glActiveTexture(GL_TEXTURE2);
					glBindTexture(GL_TEXTURE_2D, normalMap->id);
					forward_shader->setUniform("NormalMap", 2);
					hasNormalMap = true;
				}
			}
			forward_shader->setUniform("UseNormalMap", hasNormalMap);

			glDrawElementsInstanced(GL_TRIANGLES,
				static_cast<GLsizei>(indexCount),
				GL_UNSIGNED_INT,
				nullptr,
				static_cast<GLsizei>(batch.currentInstanceCount));

			if (isCheckerboard) {
				glCullFace(GL_BACK);
			}
		}

		// --- RENDER SKYBOX ---
		glDepthFunc(GL_LEQUAL);
		glDepthMask(GL_FALSE);
		glDisable(GL_CULL_FACE);

		forward_shader->setUniform("Pass", 1);

		auto skybox_view = registry.view<MeshComponent, SkyboxRenderPass>();
		for (auto [entity, mesh] : skybox_view.each())
		{
			if (meshCache.find(mesh.meshName) == meshCache.end()) {
				LoadAndCacheMesh(mesh.meshName);
			}
			auto& [vao, indexCount] = meshCache[mesh.meshName];
			if (registry.all_of<TextureComponent>(entity))
			{
				auto& texComp = registry.get<TextureComponent>(entity);
				auto texture = AssetManagerSystem::GetResource<Scene::Texture>(texComp.texture_guid);

				if (texture && texture->id != 0)
				{
					glActiveTexture(GL_TEXTURE1);
					glBindTexture(GL_TEXTURE_CUBE_MAP, texture->id);
					forward_shader->setUniform("CubeMapTex", 1);
				}
			}

			glBindVertexArray(vao);
			glDrawElements(GL_TRIANGLES, (GLsizei)indexCount, GL_UNSIGNED_INT, 0);
		}

		// --- CLEANUP ---
		glBindVertexArray(0);
		forward_shader->disuse();
		glDepthFunc(GL_LESS);
		glEnable(GL_CULL_FACE);
		glDepthMask(GL_TRUE);
	}

	void GraphicsSystem::RenderScene()
	{
		glEnable(GL_DEPTH_TEST);

		// Get camera information first
		auto camera_view = registry.view<CameraComponent>();
		if (camera_view.empty()) {
			LOGI("No cameras in registry yet, skipping render");
			return;
		}
		CameraComponent* active_camera = &camera_view.get<CameraComponent>(camera_view.front());
		glm::mat4 V = Camera::getLookAt(*active_camera);
		float aspect = (screen.y > 0) ? (float)screen.x / screen.y : 1.0f;
		glm::mat4 P = Camera::getPerspective(*active_camera, aspect);

		m_instancingSystem->PrepareBatches();
		//m_instancingSystem->PrintStatistics();

		LOGI("\n========== RENDER FRAME START ==========");

		// Step 1: Geometry Pass
		LOGI(">>> Starting Geometry Pass");
		GeometryPass(V, P);
		LOGI("<<< Geometry Pass Complete");

		// Step 2: Lighting Pass (WITH FOG INTEGRATED)
		LOGI(">>> Starting Lighting Pass (with fog)");
		LightingPass(V);
		LOGI("<<< Lighting Pass Complete");

		// Step 3: Forward Render Pass
		LOGI(">>> Starting Forward Render Pass");
		// Determine target FBO
		ForwardRenderPass(V, P);
		LOGI("<<< Forward Render Pass Complete");

		// Step 4: Copy to screen if not rendering to FBO
		if (!m_isRenderingToFBO) {
			LOGI(">>> Copying result to screen");
			glBindFramebuffer(GL_READ_FRAMEBUFFER, mainFBO.handle);
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
			glBlitFramebuffer(
				0, 0, static_cast<GLuint>(screen.x), static_cast<GLuint>(screen.y),
				0, 0, static_cast<GLuint>(screen.x), static_cast<GLuint>(screen.y),
				GL_COLOR_BUFFER_BIT, GL_NEAREST
			);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			LOGI("<<< Copy Complete");
		}

		LOGI("========== RENDER FRAME END ==========\n");
		
	}

	void GraphicsSystem::LoadAndCacheMesh(const std::string& meshName)
	{
		Mesh::MeshData meshData; // Temporary struct to hold raw mesh data

		if (meshName == "Quad") Mesh::generateQuad(meshData);
		else if (meshName == "Plane") Mesh::generatePlane(meshData);
		else if (meshName == "Cube") Mesh::generateCube(meshData);
		else if (meshName == "Sphere") Mesh::generateSphere(meshData);
		else if (meshName == "SkyBox") Mesh::generateSkyBox(meshData);
		else {
			// Here you would load from a file, e.g., using your old Mesh::generate
			// For now, we'll throw an error for unknown primitives.

			// Try to see if file is valid
			try {
				Mesh::MeshData meshFile(meshName, true);
				meshData = std::move(meshFile);
			}
			// File name is invalid
			catch (const std::exception& e) {
				throw std::runtime_error("Failed to load mesh: " + meshName + ". Error: " + e.what());
			}
		}

		if (meshData.indices.empty() || meshData.points.empty()) {
			return;
		}

		GLuint vao = 0, indexBuf = 0, posBuf = 0, normBuf = 0;
		std::vector<GLuint> buffers;

		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);

		glGenBuffers(1, &indexBuf);
		buffers.push_back(indexBuf);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuf);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, meshData.indices.size() * sizeof(GLuint), meshData.indices.data(), GL_STATIC_DRAW);

		glGenBuffers(1, &posBuf);
		buffers.push_back(posBuf);
		glBindBuffer(GL_ARRAY_BUFFER, posBuf);
		glBufferData(GL_ARRAY_BUFFER, meshData.points.size() * sizeof(GLfloat), meshData.points.data(), GL_STATIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(0);
		
		// Setup normals
		if (!meshData.normals.empty()) {
			glGenBuffers(1, &normBuf);
			buffers.push_back(normBuf);
			glBindBuffer(GL_ARRAY_BUFFER, normBuf);
			glBufferData(GL_ARRAY_BUFFER, meshData.normals.size() * sizeof(GLfloat), meshData.normals.data(), GL_STATIC_DRAW);
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
			glEnableVertexAttribArray(1);
		}

		// Setup texture coordinates
		GLuint tcBuf = 0;
		if (!meshData.texCoords.empty()) {
			glGenBuffers(1, &tcBuf);
			buffers.push_back(tcBuf);
			glBindBuffer(GL_ARRAY_BUFFER, tcBuf);
			glBufferData(GL_ARRAY_BUFFER, meshData.texCoords.size() * sizeof(GLfloat), meshData.texCoords.data(), GL_STATIC_DRAW);
			glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);
			glEnableVertexAttribArray(2);
		}

		// Setup tangents 
		GLuint tangentBuf = 0;
		if (!meshData.tangents.empty()) {
			glGenBuffers(1, &tangentBuf);
			buffers.push_back(tangentBuf);
			glBindBuffer(GL_ARRAY_BUFFER, tangentBuf);
			glBufferData(GL_ARRAY_BUFFER, meshData.tangents.size() * sizeof(GLfloat), meshData.tangents.data(), GL_STATIC_DRAW);
			glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, 0, 0);
			glEnableVertexAttribArray(3);
		}

		glBindVertexArray(0);

		// Store the VAO and index count in the cache
		meshCache[meshName] = { vao, meshData.indices.size() };

		// Note: The raw buffer handles (indexBuf, posBuf, etc.) should be stored somewhere
		// to be deleted when the mesh is unloaded. For simplicity, this is omitted here.

		// Build edges using helper function
		std::vector<uint32_t> wireIdx = BuildUniqueEdgeIndices(meshData.indices);

		// Create wire VAO and EBO
		GLuint wireVAO = 0, wireEBO = 0;
		glGenVertexArrays(1, &wireVAO);
		glBindVertexArray(wireVAO);

		glBindBuffer(GL_ARRAY_BUFFER, posBuf);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

		glGenBuffers(1, &wireEBO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, wireEBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER,
			wireIdx.size() * sizeof(GLuint),
			wireIdx.data(),
			GL_STATIC_DRAW);

		glBindVertexArray(0);

		// Store in wire cache
		meshCacheWire[meshName] = { wireVAO, wireIdx.size() };
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
		gBufferFBO.resize(window_resize.width, window_resize.height);

		screen.x = static_cast<float>(window_resize.width);
		screen.y = static_cast<float>(window_resize.height);

		// Restore previous texture bindings
		glBindTexture(GL_TEXTURE_2D, prevTexture);
	}

	void GraphicsSystem::CalculateModelMatrix(const TransformComponent& transform, glm::mat4& model)
	{
		model = glm::mat4(1.0f);
		model = glm::translate(model, transform.position);
		model = glm::rotate(model, glm::radians(transform.rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(transform.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::rotate(model, glm::radians(transform.rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::scale(model, transform.scale);
	}

	void GraphicsSystem::InitializeFullscreenQuad() {
		// Vertex data for a fullscreen quad (NDC coordinates)
		float quadVertices[] = {
			// positions   // texCoords
			-1.0f,  1.0f,  0.0f, 1.0f,
			-1.0f, -1.0f,  0.0f, 0.0f,
			 1.0f,  1.0f,  1.0f, 1.0f,
			 1.0f, -1.0f,  1.0f, 0.0f,
		};

		// Create buffers
		glGenVertexArrays(1, &fullscreenQuadVAO);
		glGenBuffers(1, &fullscreenQuadVBO);
		glBindVertexArray(fullscreenQuadVAO);
		glBindBuffer(GL_ARRAY_BUFFER, fullscreenQuadVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);

		// Position attribute
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
		// TexCoords attribute
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

		glBindVertexArray(0);
	}

}