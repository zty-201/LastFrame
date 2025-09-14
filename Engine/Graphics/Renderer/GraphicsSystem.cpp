#include "pch.hpp"
#include "global.hpp"
#include "GraphicsSystem.h"
#include "Input/InputManager.hpp"

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

        // Debug context setup for OpenGL if in debug mode
#if defined( _DEBUG )
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
#endif

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

        
            // Define the scene and its properties
		    new (&scene) Scene
            {
                // Vertex shader
                {
                    #include "../Extern/Graphics/Shader/BasicVert.vert"
                },

                // Fragment shader
                {
                    #include "../Extern/Graphics/Shader/BasicFrag.frag"
                },

                // Rendering passes
                {
                    // Pass 0
                    {
                        // Rendering target
                        { FBO::Type::Deferred, WIDTH, HEIGHT },

                        // Viewport
                        { 0, 0, WIDTH, HEIGHT },

                        // Background color
                        BLACK,

                        // Depth buffer
                        ENABLE,

                        // Objects
                        {
                            {   "Extern/bs_ears.obj",
                                {
                                    INSTANCES(-10.0f),
                                    INSTANCES(-8.0f),
                                    INSTANCES(-6.0f),
                                    INSTANCES(-4.0f),
                                    INSTANCES(-2.0f),
                                    INSTANCES(0.0f),
                                    INSTANCES(2.0f),
                                    INSTANCES(4.0f),
                                    INSTANCES(6.0f),
                                    INSTANCES(8.0f),
                                    INSTANCES(10.0f),
                                }
                            }
                        },

                // The camera
                {
                    { {0.0f, 4.0f, 5.0f} }
                },

                // Lights
                {
                    DEFAULT
                },

                // Textures
                {
                    { { "Extern/ogre_diffuse.png" },   GL_TEXTURE3 }, // Channel 3
                    { { "Extern/ogre_normalmap.png" }, GL_TEXTURE4 }  // Channel 4                         
                },

                // Setup uniforms in the shader
                NULL
            },

                // Pass 1
                {
                    // Rendering target
                    DEFAULT,

                    // Viewport
                    { 0, 0, WIDTH, HEIGHT },

                    // Background color
                    { },

                    // Depth buffer
                    DISABLE,

                    // Objects
                    {
                        {
                            QUAD
                        }
                    },

                // The camera
                {

                },

                // Lights
                {

                },

                // Textures
                {

                },

                // Setup uniforms in the shader

                }
            }
            };

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
        InputManager::RegisterCallbacks(window);

        // Draw scene
        scene.render();
    }

    void GraphicsSystem::PostUpdate(float deltaTime)
    {
        UNREFERENCED_PARAMETER(deltaTime);
        // Swap buffers
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
}