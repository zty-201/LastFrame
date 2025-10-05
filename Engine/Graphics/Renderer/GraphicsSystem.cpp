/******************************************************************/
/*!
\file   GraphicsSystem.cpp
\author Zhang TongYan
\par    tongyan.zhang\@digipen.edu
\date   October 2025
\brief
This file contains the implementation of the GraphicsSystem class, which
manages the initialization, rendering, and updating of the graphics pipeline.
It includes functionality for handling OpenGL context setup, rendering scenes,
managing framebuffers, and responding to window and level editor events.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/******************************************************************/

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
		//Creation of framebuffer for scene, used for level editor (or others).
		//Create a framebuffer for the scene to be drawn on, and read texture handle for the texturebuffer attached.
		FBO tempFBO(FBO::Texture, WIDTH, HEIGHT, &sceneTextureID);
		scene_framebuffer = std::move(tempFBO);
		shader_guid = AssetMgr::GetFullGuid(AssetMgr::SHADER_GUID, "Basic");
		pScene = std::make_unique<Scene>();
		// Scene-wide properties (can also be a component on a scene settings entity)
		pScene->fogColor = { 0.0f, 0.0f, 0.0f };
		pScene->fogMinDist = 5.0f;
		pScene->fogMaxDist = 15.0f;
		pScene->shadingModel = 0;
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

		// --- 1. SETUP THE RENDER TARGET AND STATE ---
		GLuint targetFBO = m_isRenderingToFBO ? scene_framebuffer.handle : 0;
		glBindFramebuffer(GL_FRAMEBUFFER, targetFBO);
		glViewport(0, 0, static_cast<GLsizei>(screen.x), static_cast<GLsizei>(screen.y));

		glClearColor(0.1f, 0.1f, 0.2f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// --- 2. ACTIVATE SHADER AND SET GLOBAL UNIFORMS ---
		std::shared_ptr<ShaderProgram> shader{};
		if (shader_guid != xresource::full_guid{})
		{
			shader = EngineFramework::AssetManagerSystem::GetResource<ShaderProgram>(shader_guid);
		}
		else
		{
			using E_AM = EngineFramework::AssetManagerSystem;
			shader_guid = E_AM::GetFullGuid(E_AM::SHADER_GUID, "Basic");
			shader = E_AM::GetResource<ShaderProgram>(shader_guid);
		}
		if (!shader) {
			throw std::runtime_error("[GraphicsSystem] Shader Program not loaded or Shader guid is invalid.");
		}
		shader->use();

		shader->setUniform("Pass", 0);
		shader->setUniform("fog.color", pScene->fogColor);
		shader->setUniform("fog.minDist", pScene->fogMinDist);
		shader->setUniform("fog.maxDist", pScene->fogMaxDist);
		shader->setUniform("materialType", pScene->shadingModel);

		// Find and set camera uniforms
		auto camera_view = registry.view<CameraComponent>();
		if (camera_view.empty()) {
			LOGI("No cameras in registry yet, skipping render");
			shader->disuse();
			return;
		}
		CameraComponent* active_camera = nullptr;
		active_camera = &camera_view.get<CameraComponent>(camera_view.front());

		if (active_camera) {
			Camera::setUniforms(*active_camera, shader);
			glm::mat4 V = Camera::getLookAt(*active_camera);
			float aspect = (screen.y > 0) ? (float)screen.x / screen.y : 1.0f;
			glm::mat4 P = Camera::getPerspective(*active_camera, aspect);
			shader->setUniform("V", V);
			shader->setUniform("P", P);
		}

		// Find and set light uniforms
		auto light_view = registry.view<LightComponent>();
		int light_index = 0;
		for (auto entity : light_view) {
			auto& light = light_view.get<LightComponent>(entity);
			Light::setUniforms(light, shader, light_index++);
		}

		// --- 3. RENDER SCENE OBJECTS ---
		glDepthFunc(GL_LESS);
		glDepthMask(GL_TRUE);
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);

		auto scene_view = registry.view<TransformComponent, MaterialComponent, MeshComponent, SceneRenderPass>();
		for (auto [entity, transform, material, mesh] : scene_view.each())
		{
			// Load mesh if not cached
			if (meshCache.find(mesh.meshName) == meshCache.end()) {
				LoadAndCacheMesh(mesh.meshName);
			}
			auto& [vao, indexCount] = meshCache[mesh.meshName];

			glBindVertexArray(vao);

			bool isCheckerboard = (material.effect == 3.0f);
			if (isCheckerboard) {
				glCullFace(GL_FRONT);
			}

			// **NEW: Check for texture and bind it**
			bool hasTexture = false;
			if (registry.all_of<TextureComponent>(entity))
			{
				auto& texComp = registry.get<TextureComponent>(entity);
				auto texture = AssetManagerSystem::GetResource<Scene::Texture>(texComp.texture_guid);

				if (texture && texture->id != 0)
				{
					glActiveTexture(GL_TEXTURE0); // Diffuse always on unit 0
					glBindTexture(texture->target, texture->id);
					shader->setUniform("DiffuseTex", 0);
					hasTexture = true;
				}
			}
			shader->setUniform("UseTexture", hasTexture);

			// Bind normal map (separate check)
			bool hasNormalMap = false;
			if (registry.all_of<NormalMapComponent>(entity))
			{
				auto& normalComp = registry.get<NormalMapComponent>(entity);
				auto normalMap = AssetManagerSystem::GetResource<Scene::Texture>(normalComp.normalmap_guid);

				if (normalMap && normalMap->id != 0)
				{
					glActiveTexture(GL_TEXTURE2); // Normal map on unit 2
					glBindTexture(GL_TEXTURE_2D, normalMap->id);
					shader->setUniform("NormalMap", 2);
					hasNormalMap = true;
				}
			}
			shader->setUniform("UseNormalMap", hasNormalMap);

			shader->setMaterialUniforms(material);
			CalculateModelMatrix(transform, transform.modelMatrix);
			shader->setUniform("M", transform.modelMatrix);

			glDrawElements(GL_TRIANGLES, (GLsizei)indexCount, GL_UNSIGNED_INT, 0);

			if (isCheckerboard) {
				glCullFace(GL_BACK);
			}

			// --- 3.5. Debug Drawing ---
			if (show_debugDraw) {
				// 1) Ensure a wire VAO exists for this mesh
				auto itWire = meshCacheWire.find(mesh.meshName);
				if (itWire == meshCacheWire.end()) {
					// Rebuild cache entries (creates wire VAO/EBO too)
					LoadAndCacheMesh(mesh.meshName);
					itWire = meshCacheWire.find(mesh.meshName);
				}
				if (itWire == meshCacheWire.end())
					break; // nothing to draw for this mesh

				const auto& [wireVAO, wireIndexCount] = itWire->second;

				// 2) Save minimal GL state we’re about to touch
				const GLboolean depthEnabled = glIsEnabled(GL_DEPTH_TEST);
				const GLboolean cullEnabled = glIsEnabled(GL_CULL_FACE);

				// 3) Overlay: draw lines without depth testing so they’re always visible
				glDisable(GL_DEPTH_TEST);
				glDisable(GL_CULL_FACE);
				glLineWidth(1.0f);                 // ES often clamps to 1.0

				shader->setUniform("Pass", 2);     // flat debug branch in your frag shader
				shader->setUniform("M", transform.modelMatrix);

				glBindVertexArray(wireVAO);
				glDrawElements(GL_LINES, (GLsizei)wireIndexCount, GL_UNSIGNED_INT, 0);
				glBindVertexArray(0);

				// 4) Restore shader + GL state
				shader->setUniform("Pass", 0);
				if (cullEnabled)  glEnable(GL_CULL_FACE);  else glDisable(GL_CULL_FACE);
				if (depthEnabled) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
			}
		}

		// --- 4. RENDER SKYBOX ---
		glDepthFunc(GL_LEQUAL);
		glDepthMask(GL_FALSE);
		glDisable(GL_CULL_FACE);

		shader->setUniform("Pass", 1);

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
					shader->setUniform("CubeMapTex", 1);
				}
			}
			

			glBindVertexArray(vao);
			glDrawElements(GL_TRIANGLES, (GLsizei)indexCount, GL_UNSIGNED_INT, 0);
		}

		// --- FINAL CLEANUP ---
		glBindVertexArray(0);
		shader->disuse();
		glDepthFunc(GL_LESS);
		glEnable(GL_CULL_FACE);
		glDepthMask(GL_TRUE);
		if (targetFBO != 0) {
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}
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

		// Resize framebuffer
		scene_framebuffer.resize(window_resize.width, window_resize.height);
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

}