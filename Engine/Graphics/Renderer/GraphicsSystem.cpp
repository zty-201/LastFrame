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
		shader_guid = AssetMgr::GetFullGuid(AssetMgr::SHADER_GUID, "Basic");
		geometric_shader_guid = AssetMgr::GetFullGuid(AssetMgr::SHADER_GUID, "Geometric");
		lighting_shader_guid = AssetMgr::GetFullGuid(AssetMgr::SHADER_GUID, "Lighting");

		// Initialize the fullscreen quad for the lighting pass
		InitializeFullscreenQuad();

		// Initialize skinning UBO
		glGenBuffers(1, &skinningUBO);
		glBindBuffer(GL_UNIFORM_BUFFER, skinningUBO);
		glBufferData(GL_UNIFORM_BUFFER, sizeof(glm::mat4) * maxBones, nullptr, GL_DYNAMIC_DRAW);
		glBindBufferBase(GL_UNIFORM_BUFFER, 0, skinningUBO); // binding = 0 in your shader
		glBindBuffer(GL_UNIFORM_BUFFER, 0);

		// Other setup code...
		pScene = std::make_unique<Scene>();
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
		if (geometric_shader_guid != xresource::full_guid{})
		{
			geometric_shader = EngineFramework::AssetManagerSystem::GetResource<ShaderProgram>(geometric_shader_guid);
		}
		else
		{
			using E_AM = EngineFramework::AssetManagerSystem;
			geometric_shader_guid = E_AM::GetFullGuid(E_AM::SHADER_GUID, "Geometric");
			geometric_shader = E_AM::GetResource<ShaderProgram>(geometric_shader_guid);
		}

		geometric_shader->use();

		// Set camera and view/projection matrices
		geometric_shader->setUniform("V", V);
		geometric_shader->setUniform("P", P);

		// Enable depth test and face culling
		glDepthFunc(GL_LESS);
		glDepthMask(GL_TRUE);
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);

		// Render all opaque scene objects to G-buffer
		auto scene_view = registry.view<TransformComponent, MaterialComponent, MeshComponent, SceneRenderPass>();
		for (auto [entity, transform, material, mesh] : scene_view.each())
		{
			// Skip transparent/special effect objects (handle in forward pass)
			if (material.effect == 1.0f) continue; // Skip objects with discard

			// Load mesh if not cached
			if (meshCache.find(mesh.meshName) == meshCache.end()) {
				LoadAndCacheMesh(mesh.meshName);
			}
			auto& [vao, indexCount] = meshCache[mesh.meshName];

			glBindVertexArray(vao);

			// Handle textures
			bool hasTexture = false;
			if (registry.all_of<TextureComponent>(entity))
			{
				auto& texComp = registry.get<TextureComponent>(entity);
				auto texture = AssetManagerSystem::GetResource<Scene::Texture>(texComp.texture_guid);

				if (texture && texture->id != 0)
				{
					glActiveTexture(GL_TEXTURE0);
					glBindTexture(texture->target, texture->id);
					geometric_shader->setUniform("DiffuseTex", 0);
					hasTexture = true;
				}
			}
			geometric_shader->setUniform("UseTexture", hasTexture);

			// Handle normal maps
			bool hasNormalMap = false;
			if (registry.all_of<NormalMapComponent>(entity))
			{
				auto& normalComp = registry.get<NormalMapComponent>(entity);
				auto normalMap = AssetManagerSystem::GetResource<Scene::Texture>(normalComp.normalmap_guid);

				if (normalMap && normalMap->id != 0)
				{
					glActiveTexture(GL_TEXTURE2);
					glBindTexture(GL_TEXTURE_2D, normalMap->id);
					geometric_shader->setUniform("NormalMap", 2);
					hasNormalMap = true;
				}
			}
			geometric_shader->setUniform("UseNormalMap", hasNormalMap);

			// Set material and transformation
			geometric_shader->setUniform("material.color", material.color);
			geometric_shader->setUniform("material.metal", material.metal);
			geometric_shader->setUniform("material.rough", material.rough);

			CalculateModelMatrix(transform, transform.modelMatrix);
			geometric_shader->setUniform("M", transform.modelMatrix);

			// Draw the mesh
			glDrawElements(GL_TRIANGLES, (GLsizei)indexCount, GL_UNSIGNED_INT, 0);
		}
	}

	void GraphicsSystem::LightingPass(glm::mat4& V)
	{

		//-------------------------------------------------------------------------
		// LIGHTING PASS - Calculate lighting using G-buffer data
		//-------------------------------------------------------------------------
		auto light_view = registry.view<LightComponent>();
		GLuint targetFBO = m_isRenderingToFBO ? mainFBO.handle : 0;
		glBindFramebuffer(GL_FRAMEBUFFER, targetFBO);
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
		lighting_shader->setUniform("materialType", pScene->shadingModel);
		lighting_shader->setUniform("fog.color", pScene->fogColor);
		lighting_shader->setUniform("fog.minDist", pScene->fogMinDist);
		lighting_shader->setUniform("fog.maxDist", pScene->fogMaxDist);
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

		// Draw fullscreen quad to apply lighting
		glDisable(GL_DEPTH_TEST);
		glBindVertexArray(fullscreenQuadVAO);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		glEnable(GL_DEPTH_TEST);

		// Copy depth buffer from G-buffer to main framebuffer for forward rendering
		glBindFramebuffer(GL_READ_FRAMEBUFFER, gBufferFBO.handle);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, targetFBO);
		glBlitFramebuffer(
			0, 0, static_cast<GLuint>(screen.x), static_cast<GLuint>(screen.y),
			0, 0, static_cast<GLuint>(screen.x), static_cast<GLuint>(screen.y),
			GL_DEPTH_BUFFER_BIT, GL_NEAREST
		);
		glBindFramebuffer(GL_FRAMEBUFFER, targetFBO);
	}

	void GraphicsSystem::ForwardRenderPass(glm::mat4& V, glm::mat4& P, GLuint targetFBO)
	{
		//-------------------------------------------------------------------------
		// FORWARD PASS - Render transparent objects and skybox
		//-------------------------------------------------------------------------
		auto light_view = registry.view<LightComponent>();
		// Use the forward rendering shader for transparent objects and skybox

		std::shared_ptr<ShaderProgram> basic_shader{};
		if (shader_guid != xresource::full_guid{})
		{
			basic_shader = EngineFramework::AssetManagerSystem::GetResource<ShaderProgram>(shader_guid);
		}
		else
		{
			using E_AM = EngineFramework::AssetManagerSystem;
			shader_guid = E_AM::GetFullGuid(E_AM::SHADER_GUID, "Basic");
			basic_shader = E_AM::GetResource<ShaderProgram>(shader_guid);
		}

		basic_shader->use();

		// Set global uniforms
		basic_shader->setUniform("Pass", 0);
		basic_shader->setUniform("fog.color", pScene->fogColor);
		basic_shader->setUniform("fog.minDist", pScene->fogMinDist);
		basic_shader->setUniform("fog.maxDist", pScene->fogMaxDist);
		basic_shader->setUniform("materialType", pScene->shadingModel);
		basic_shader->setUniform("V", V);
		basic_shader->setUniform("P", P);

		// Set light uniforms
		int light_index = 0;
		for (auto entity : light_view) {
			auto& light = light_view.get<LightComponent>(entity);
			Light::setUniforms(light, basic_shader, light_index++);
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
					basic_shader->setUniform("DiffuseTex", 0);
					hasTexture = true;
				}
			}
			basic_shader->setUniform("UseTexture", hasTexture);

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
					basic_shader->setUniform("NormalMap", 2);
					hasNormalMap = true;
				}
			}
			basic_shader->setUniform("UseNormalMap", hasNormalMap);

			basic_shader->setMaterialUniforms(material);
			CalculateModelMatrix(transform, transform.modelMatrix);
			basic_shader->setUniform("M", transform.modelMatrix);

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

				basic_shader->setUniform("Pass", 2);     // flat debug branch in your frag shader
				basic_shader->setUniform("M", transform.modelMatrix);

				glBindVertexArray(wireVAO);
				glDrawElements(GL_LINES, (GLsizei)wireIndexCount, GL_UNSIGNED_INT, 0);
				glBindVertexArray(0);

				// 4) Restore shader + GL state
				basic_shader->setUniform("Pass", 0);
				if (cullEnabled)  glEnable(GL_CULL_FACE);  else glDisable(GL_CULL_FACE);
				if (depthEnabled) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
			}
		}

		// --- 4. RENDER SKYBOX ---
		glDepthFunc(GL_LEQUAL);
		glDepthMask(GL_FALSE);
		glDisable(GL_CULL_FACE);

		basic_shader->setUniform("Pass", 1);

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
					basic_shader->setUniform("CubeMapTex", 1);
				}
			}


			glBindVertexArray(vao);
			glDrawElements(GL_TRIANGLES, (GLsizei)indexCount, GL_UNSIGNED_INT, 0);
		}

		// --- FINAL CLEANUP ---
		glBindVertexArray(0);
		basic_shader->disuse();
		glDepthFunc(GL_LESS);
		glEnable(GL_CULL_FACE);
		glDepthMask(GL_TRUE);
		if (targetFBO != 0) {
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}
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

		// Geometry Pass
		GeometryPass(V, P);
		// Lighting Pass
		LightingPass(V);
		// Forward Render Pass
		GLuint targetFBO = m_isRenderingToFBO ? mainFBO.handle : 0;
		ForwardRenderPass(V, P, targetFBO);
		
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

		GLuint animBuf = 0;
		if (!meshData.boneWeights.empty()) {
			glGenBuffers(1, &animBuf);
			buffers.push_back(animBuf);
			glBindBuffer(GL_ARRAY_BUFFER, animBuf);
			glBufferData(GL_ARRAY_BUFFER, meshData.boneWeights.size() * sizeof(uint8_t), meshData.boneWeights.data(), GL_STATIC_DRAW);
			glEnableVertexAttribArray(4);
			glVertexAttribPointer(4, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, 0);
		}

		GLuint indiBuf = 0;
		if (!meshData.boneIndices.empty()) {
			glGenBuffers(1, &indiBuf);
			buffers.push_back(indiBuf);
			glBindBuffer(GL_ARRAY_BUFFER, indiBuf);
			glBufferData(GL_ARRAY_BUFFER, meshData.boneIndices.size() * sizeof(uint8_t), meshData.boneIndices.data(), GL_STATIC_DRAW);
			glEnableVertexAttribArray(5);
			glVertexAttribIPointer(5, 4, GL_UNSIGNED_BYTE, GL_FALSE, 0, 0);
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

	void GraphicsSystem::UploadBonePalettes(const std::vector<glm::mat4>& boneMatrices)
	{
		glBindBuffer(GL_UNIFORM_BUFFER, skinningUBO);
		glBufferSubData(GL_UNIFORM_BUFFER, 0, boneMatrices.size() * sizeof(glm::mat4), boneMatrices.data());
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
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