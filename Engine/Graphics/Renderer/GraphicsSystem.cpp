#include "pch.hpp"
#include "global.hpp"
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
		//Do not use sceneFBO anymore, its contents have been written to the member variable.
		scene_framebuffer = std::move(tempFBO);

		//TODO: ==Hardcoded scene, for demo only.==

		//Scene Properties
		fogColor = { 0.0f, 0.0f, 0.0f }; // Example: gray fog color
		fogMinDist = 5.0f;
		fogMaxDist = 15.0f;
		shadingModel = 0;
		Camera camera = { { 0.0f, 1.0f, 5.0f }, 75.0f };
		pScene = std::make_unique<Scene>(
			AssetMgr::GetFullGuid(AssetMgr::SHADER_GUID, "Basic"), 
			std::initializer_list<Scene::Pass>{         // List of rendering passes
			// Pass 0: Render Scene
			Scene::Pass{
				DEFAULT,
				{ 0, 0, WIDTH, HEIGHT },            // Viewport
				GL_LESS,                            // Depth test LESS for the scene
				{                                   // Objects
					{ PLANE, MATERIAL_CHECKERBOARD, glm::translate(glm::scale(glm::mat4(1.0f), {10.0f, 1.0f, 10.0f}), {0.0f, -1.01f, 0.0f}) },
					{ SPHERE, MATERIAL_PBR_PHONG, glm::translate(glm::mat4(1.0f), {-3.0f, 0.0f, 0.0f}) },
					{ CUBE, MATERIAL_DISCARD, glm::translate(glm::mat4(1.0f), {3.0f, 0.0f, 0.0f}) },
					//{ "Extern/tree.obj", MATERIAL_CARTOON, glm::translate(glm::mat4(1.0f), {0.0f, 0.0f, 0.0f}) }
				},
				{ camera },                         // Cameras
				{ { {0.0f, 5.0f, 2.0f} } },         // Lights
				{},                                 // Textures
				[this](ShaderProgram& shader) {     // Uniform callback
					shader.setUniform("fog.color", fogColor);
					shader.setUniform("fog.minDist", fogMinDist);
					shader.setUniform("fog.maxDist", fogMaxDist);
					shader.setUniform("materialType", shadingModel);
				}
			},
			// Pass 1: Render Skybox
			Scene::Pass{
				DEFAULT,      // Default FBO
				{ 0, 0, WIDTH, HEIGHT },            // Viewport
				GL_LEQUAL,                          // Depth test LEQUAL for the skybox
				{                                   // Objects
					{ SKYBOX }
				},
				{ camera },                         // Cameras
				{},                                 // Lights
				{ AssetMgr::GetFullGuid(AssetMgr::CUBEMAP_GUID, "night_skybox")},                        // Textures
				nullptr                             // Uniform callback
			}
			}
		);
	}

	void GraphicsSystem::Update(float deltaTime)
	{
		UNREFERENCED_PARAMETER(deltaTime);
		// Draw scene
		pScene->render();
	}

	void GraphicsSystem::PostUpdate(float deltaTime)
	{
		UNREFERENCED_PARAMETER(deltaTime);
#ifdef PLATFORM_WINDOWS
		// Swap buffers
		glfwSwapBuffers(pWindow);
#endif
	}


	//Toggle drawing to framebuffer.
	void GraphicsSystem::DrawSceneToFramebuffer(bool is_draw_to_framebuffer)
	{
		/*
			TODO:
			As of 22/09/2025, the rendering process is as follows
			pScene->render() --> renders each pass sequentially.
			Pass0: Renders objects to screen.
			Pass1: Renders skybox to screen where there are no fragments with depth set.

			Pass 1 does not use Pass0 texture, and thus both passes need to be binded to the framebuffer.

			If there is a case where subsequent passes carries over framebuffer texture from previous passes, then only bind the last pass to the framebuffer.
		*/
		//Get the last stage of the drawing process.
		std::vector<Scene::Pass>& passes = pScene->passes;
		//If set, the scene will draw to the framebuffer.
		if (is_draw_to_framebuffer)
		{
			for (Scene::Pass& pass : passes)
			{
				pass.SetFBO(scene_framebuffer);
			}
		}
		else
		{
			for (Scene::Pass& pass : passes)
			{
				//If no param passed in, the scene will not draw to a framebuffer but rather to the window.
				pass.SetFBO();
			}
		}
	}

	//==Event Handlers==//

	//When level editor is toggled on/off.
	void GraphicsSystem::HANDLE_EVENT_LevelEditorToggled(Event_Level_Editor_Toggled& editor_toggled)
	{
		//Only draw the scene to the framebuffer if in level editor.
		if (editor_toggled.is_enabled)
		{
			DrawSceneToFramebuffer(true);
		}
		else
		{
			DrawSceneToFramebuffer(false);
		}
	}

	//When drawing area should be resized.
	void GraphicsSystem::HANDLE_EVENT_WindowResized(Event_Window_Resized& window_resize)
	{
		pScene->resize(window_resize.width, window_resize.height);
	}

}