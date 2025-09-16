#include "pch.hpp"
#include "global.hpp"
#include "GraphicsSystem.h"
#include "InputManager.hpp"


#ifdef _DEBUG
#include "System_LevelEditor.hpp"
class LevelEditor; //forward declaration
#endif



namespace EngineFramework
{
    GraphicsSystem::GraphicsSystem()
    {
    }

    GraphicsSystem::~GraphicsSystem()
    {
        // Clean up GLFW
        glfwTerminate();
    }

    void GraphicsSystem::ErrorCallback(int error, const char* msg)
    {
		UNREFERENCED_PARAMETER(error);
        fputs(msg, stderr);
    }

    void GraphicsSystem::Initialize()
    {
        SetupGLFW();
        SetupImGui();
        SetupGraphics();
        //need to have a function for input handling
        //since input handling is part of glfw , at least for the callbacks
    }

    void GraphicsSystem::SetupGLFW()
    {
        // Initialize GLFW library
        if (!glfwInit()) {
            throw std::runtime_error("Failed to initialize GLFW");
        }

        // Configure OpenGL context properties
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);

        window = glfwCreateWindow(WIDTH, HEIGHT, "Framework", NULL, NULL);

        if (!window) {
            glfwTerminate();
            throw std::runtime_error("Failed to create GLFW window");
        }

        glfwMakeContextCurrent(window);
        //glfwSwapInterval(1); // Enable vsync

        // Initialize GLAD (must happen after window context is created)
        if (!gladLoadGLES2Loader((GLADloadproc)glfwGetProcAddress)) {
            glfwTerminate();
            throw std::runtime_error("Failed to initialize GLAD");
        }


        // Print OpenGL info
        printf("OpenGL Version: %s\n", glGetString(GL_VERSION));
        printf("GLSL Version: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
        printf("Rendered by: %s\n", glGetString(GL_RENDERER));
        

    }

    void GraphicsSystem::SetupImGui()
    {
        //imgui should not be an entity of graphics
        /*ImGui::CreateContext();
        ImGui_ImplGlfw_InitForOpenGL(scene.window, true);
        ImGui_ImplOpenGL3_Init();*/
    }

    void GraphicsSystem::SetupGraphics()
    { 
        fogColor = { 0.0f, 0.0f, 0.0f }; // Example: gray fog color
        fogMinDist = 5.0f;
        fogMaxDist = 15.0f;
        shadingModel = 0;
		// Define the scene and its properties
		Scene::Texture texture = {
			 { "Extern/night_negx.png", "Extern/night_posx.png",
			   "Extern/night_posy.png", "Extern/night_negy.png",
			   "Extern/night_posz.png", "Extern/night_negz.png"  }
		};

		Camera camera = { { 0.0f, 1.0f, 5.0f }, 75.0f };
        scene = std::make_unique<Scene>(
            "Extern/Graphics/Shader/BasicVert.vert", // Vertex shader file path
            "Extern/Graphics/Shader/BasicFrag.frag", // Fragment shader file path
            std::initializer_list<Scene::Pass>{         // List of rendering passes
            // Pass 0: Render Skybox
            Scene::Pass{
                {FBO::Default, WIDTH, HEIGHT},                                 // Default FBO
                { 0, 0, WIDTH, HEIGHT },           // Viewport
                { },        // Clear color
                true,                              // Enable depth buffer
                {                                  // Objects
                    { SKYBOX }
                },
                { camera },                        // Cameras
                {},                                // Lights
                { texture },                       // Textures
                nullptr                            // Uniform callback
            },
                // Pass 1: Render Scene
                Scene::Pass{
                    {FBO::Default, WIDTH, HEIGHT},                                 // Default FBO
                    { 0, 0, WIDTH, HEIGHT },           // Viewport
                    {},                                 // Clear color
                    true,                                 // Depth buffer
                    {                                  // Objects
                        { PLANE, MATERIAL_CHECKERBOARD, glm::translate(glm::scale(glm::mat4(1.0f), {10.0f, 1.0f, 10.0f}), {0.0f, -1.01f, 0.0f}) },
                        { SPHERE, MATERIAL_PBR_PHONG, glm::translate(glm::mat4(1.0f), {-3.0f, 0.0f, 0.0f}) },
                        { CUBE, MATERIAL_DISCARD, glm::translate(glm::mat4(1.0f), {3.0f, 0.0f, 0.0f}) },
                        { CUBE, MATERIAL_DISCARD, glm::translate(glm::mat4(1.0f), {3.0f, 0.0f, 0.0f}) }
                    },
                    { camera },                        // Cameras
                    { { {0.0f, 5.0f, 2.0f} } },       // Lights
                    {},                                // Textures
                    [this](ShaderProgram& shader) {       // Uniform callback
                        shader.setUniform("fog.color", fogColor);
                        shader.setUniform("fog.minDist", fogMinDist);
                        shader.setUniform("fog.maxDist", fogMaxDist);
                        shader.setUniform("materialType", shadingModel);
                    }
            }
        }
        );
		
		

    }

    void GraphicsSystem::Update(float deltaTime)
    {
		UNREFERENCED_PARAMETER(deltaTime);
        
        // Check for window close
        if (window && glfwWindowShouldClose(window))
        {
            // Signal application to close
            // For now, you can just use glfwSetWindowShouldClose
            // Later you might want a proper way to signal the CoreEngine
        }
        
        //Input Callbacks
        //When Level Editor is enabled, disable the input callbacks from core engine. Because imgui have its own sets of inputs.

#ifdef _DEBUG
        if (!LevelEditor::Get_Editor_State()) {
            InputManager::RegisterCallbacks(window);
        }
#else
        InputManager::RegisterCallbacks(window);
#endif        
        
        // Draw scene
        scene->render();
    }

    void GraphicsSystem::PostUpdate(float deltaTime)
    {
        UNREFERENCED_PARAMETER(deltaTime);
        // Swap buffers
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
}