#include "pch.hpp"
#include "global.hpp"
#include "GraphicsSystem.h"

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
        glfwSetErrorCallback(ErrorCallback);

        // Initialize GLFW first
        if (!glfwInit())
            throw std::runtime_error("Failed to initialize GLFW");

        // Set window hints
        glfwWindowHint(GLFW_RESIZABLE, 1);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, 0);
        
        // Create window
        scene.window = glfwCreateWindow(750, 750, "Game Engine", NULL, NULL);
        if (!scene.window) {
            glfwTerminate();
            throw std::runtime_error("Failed to create GLFW window");
        }

        glfwMakeContextCurrent(scene.window);
        glfwSwapInterval(1); // Enable vsync

        // Initialize GLAD (must happen after window context is created)
        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
            glfwTerminate();
            throw std::runtime_error("Failed to initialize GLAD");
        }

        // Print OpenGL info
        printf("OpenGL Version: %s\n", glGetString(GL_VERSION));
        printf("GLSL Version: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
        printf("Rendered by: %s\n", glGetString(GL_RENDERER));
        fflush(stdout);
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
        
        // Initialize the scene
        scene.InitializeScene();
    }

    void GraphicsSystem::Update(float deltaTime)
    {
		UNREFERENCED_PARAMETER(deltaTime);
        // Process GLFW events
        glfwPollEvents();
        
        // Check for window close
        if (scene.window && glfwWindowShouldClose(scene.window))
        {
            // Signal application to close
            // For now, you can just use glfwSetWindowShouldClose
            // Later you might want a proper way to signal the CoreEngine
        }
        
        
        // Draw scene
        scene.DrawScene();
    }

    void GraphicsSystem::PostUpdate(float deltaTime)
    {
        UNREFERENCED_PARAMETER(deltaTime);
        // Draw ImGui interface
        scene.DrawGUI();

        // Swap buffers
        glfwSwapBuffers(scene.window);
    }
}