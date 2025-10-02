/*
*****************************************************************/
/*!
\file   InputManager.cpp
\author Tang Yeun, Dean
\par    y.tang\@digipen.edu
\date   September 2025
\brief
This file contains the definitions of the functions that are needed for
handling the inputs in the system.
Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* *******************************************************************/
#include "pch.hpp"
#include "global.hpp"
#include "InputManager.hpp"
#include "GraphicsSystem.h"
//#include "Level_Editor.hpp"

#if defined(DEBUG)
#include "System_LevelEditor.hpp"
#endif
#include "AssetManager.hpp"



namespace EngineFramework
{
    
    // Static member definitions
    std::unordered_map<int, KeyState> InputManager::keyStates;
    std::unordered_map<int, KeyState> InputManager::mouseButtonStates;

    double InputManager::mouseX = 0.0;
    double InputManager::mouseY = 0.0;
    double InputManager::lastMouseX = 0.0;
    double InputManager::lastMouseY = 0.0;

    double InputManager::scrollX = 0.0;
    double InputManager::scrollY = 0.0;

    int InputManager::windowWidth = 0;
    int InputManager::windowHeight = 0;

#ifdef PLATFORM_WINDOWS
    void InputManager::RegisterCallbacks(GLFWwindow* window)
    {
        glfwSetKeyCallback(window, KeyCallback);
        glfwSetMouseButtonCallback(window, MouseButtonCallback);
        glfwSetCursorPosCallback(window, CursorPosCallback);
        glfwSetScrollCallback(window, ScrollCallback);
        glfwSetWindowSizeCallback(window, WindowSizeCallback);
        glfwSetErrorCallback(ErrorCallback);
    }

    void InputManager::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
    {
        (void)window;
        (void)scancode;
        (void)mods;
        auto camera_view = registry.view<CameraComponent>();
		auto& camera = camera_view.get<CameraComponent>(camera_view.front());
        //auto& pScene = GraphicsSystem::GetInstance().GetScene();
        //std::shared_ptr<ShaderProgram> shader = EngineFramework::AssetManagerSystem::GetResource<ShaderProgram>(pScene->shader_guid);
        switch (action)
        {
        case GLFW_PRESS:
            // Transition to Pressed (this frame only)
            keyStates[key] = KeyState::Pressed;
            if (key == 'W')
				Camera::setType(camera, CameraComponent::CameraType::WALKING);
            else if (key == 'O')
				Camera::setType(camera, CameraComponent::CameraType::ORBITING);
            else if (key == GLFW_KEY_1)
            {
                //shader->use();
                //shader->setUniform("materialType", 0); // PBR
            }
            else if (key == GLFW_KEY_2)
            {
                //shader->use();
                //shader->setUniform("materialType", 1); // Blinn-Phong
            }
            break;
        case GLFW_RELEASE:
            // Transition to Released (this frame only)
            keyStates[key] = KeyState::Released;
            break;
        case GLFW_REPEAT:
            // Keep it as Down (held)
            keyStates[key] = KeyState::Down;
            break;
        }
    }

    void InputManager::MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
    {
        (void)window;
        (void)mods;

        if (action == GLFW_PRESS) { 
            mouseButtonStates[button] = KeyState::Pressed; }
        else if (action == GLFW_RELEASE) { mouseButtonStates[button] = KeyState::Released; }
    }

    void InputManager::CursorPosCallback(GLFWwindow* window, double xpos, double ypos)
    {
		UNREFERENCED_PARAMETER(window);
        static double oldxpos = xpos;
        static double oldypos = ypos;
        double xoffset = xpos - oldxpos;
        double yoffset = ypos - oldypos;
        oldxpos = xpos;
        oldypos = ypos;
        auto camera_view = registry.view<CameraComponent>();
        auto& camera = camera_view.get<CameraComponent>(camera_view.front());
        if (mouseButtonStates[GLFW_MOUSE_BUTTON_LEFT] == KeyState::Pressed 
            || (mouseButtonStates[GLFW_MOUSE_BUTTON_LEFT] == KeyState::Down))
        {
#if defined(DEBUG) && defined(PLATFORM_WINDOWS)
            if (!LevelEditorSystem::Get_Enable()) {
				Camera::onCursor(camera, xoffset, yoffset);
            }
#else
            Camera::onCursor(camera, xoffset, yoffset);
#endif
        }
        else if (mouseButtonStates[GLFW_MOUSE_BUTTON_RIGHT] == KeyState::Pressed)
        {
            auto light_view = registry.view<LightComponent>();
            auto& light = light_view.get<LightComponent>(light_view.front());
#if defined(DEBUG) && defined(PLATFORM_WINDOWS)
            if (!LevelEditorSystem::Get_Enable()) {

				Light::onCursor(light, xoffset, yoffset);
            }
#else
            Light::onCursor(light, xoffset, yoffset);
#endif
        }
    }

    void InputManager::ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
    {
        (void)window;
        auto camera_view = registry.view<CameraComponent>();
        auto& camera = camera_view.get<CameraComponent>(camera_view.front());
        scrollX = xoffset;
        scrollY = yoffset;
#if defined(DEBUG) && defined(PLATFORM_WINDOWS)
        if (!LevelEditorSystem::Get_Enable()) {
			Camera::onScroll(camera, xoffset, yoffset);
        }
#else
        Camera::onScroll(camera, xoffset, yoffset);
#endif
    }

    void InputManager::WindowSizeCallback(GLFWwindow* window, int width, int height)
    {
		UNREFERENCED_PARAMETER(window);
        Event_Window_Resized window_resize;
        windowWidth = width;
        windowHeight = height;
        window_resize.width = width;
        window_resize.height = height;
        event_dispatcher.trigger<Event_Window_Resized>(std::move(window_resize));
    }
#endif
    void InputManager::ErrorCallback(int error, const char* description)
    {
        (void)error;
        LOGE("GLFW Error: %s", description);
    }

    bool InputManager::GetKey(int key)
    {
        auto it = keyStates.find(key);
        return it != keyStates.end() && (it->second == KeyState::Pressed || it->second == KeyState::Down);
    }

    bool InputManager::GetKeyDown(int key)
    {
        auto it = keyStates.find(key);
        return it != keyStates.end() && it->second == KeyState::Pressed;
    }

    bool InputManager::GetKeyUp(int key)
    {
        auto it = keyStates.find(key);
        return it != keyStates.end() && it->second == KeyState::Released;
    }

    bool InputManager::GetMouseButton(int button)
    {
        auto it = mouseButtonStates.find(button);
        return it != mouseButtonStates.end() && (it->second == KeyState::Pressed || it->second == KeyState::Down);
    }

    bool InputManager::GetMouseButtonDown(int button)
    {
        auto it = mouseButtonStates.find(button);
        return it != mouseButtonStates.end() && it->second == KeyState::Pressed;
    }

    bool InputManager::GetMouseButtonUp(int button)
    {
        auto it = mouseButtonStates.find(button);
        return it != mouseButtonStates.end() && it->second == KeyState::Released;
    }

    std::pair<double, double> InputManager::GetMousePosition()
    {
        return { mouseX, mouseY };
    }

    std::pair<double, double> InputManager::GetMouseDelta()
    {
        return { mouseX - lastMouseX, mouseY - lastMouseY };
    }

    std::pair<double, double> InputManager::GetScrollDelta()
    {
        return { scrollX, scrollY };
    }

    std::pair<int, int> InputManager::GetWindowSize()
    {
        return { windowWidth, windowHeight };
    }

    void InputManager::Initialize()
    {
        windowWidth = GraphicsSystem::GetInstance().GetWindowSize().first;
        windowHeight = GraphicsSystem::GetInstance().GetWindowSize().second;
    }

    void InputManager::Update(float deltaTime)
    {
        (void)deltaTime;
    }

    void InputManager::PostUpdate(float deltaTime)
    {
        (void)deltaTime;

        // --- Keyboard ---
        for (auto& [key, state] : keyStates)
        {
            switch (state)
            {
            case KeyState::Pressed:
                state = KeyState::Down; // Pressed → Down
                break;
            case KeyState::Released:
                state = KeyState::None; // Released → None
                break;
            case KeyState::Down:
                break;
            case KeyState::None:
                // Nothing to print
                break;
            }
        }

        // --- Mouse ---
        for (auto& [button, state] : mouseButtonStates)
        {
            switch (state)
            {
            case KeyState::Pressed:
                state = KeyState::Down; // Pressed → Down
                break;
            case KeyState::Released:
                state = KeyState::None; // Released → None
                break;
            case KeyState::Down:
                break;
            case KeyState::None:
                break;
            }
        }

        // Reset scroll deltas every frame
        scrollX = 0.0;
        scrollY = 0.0;

        // Update last mouse position
        lastMouseX = mouseX;
        lastMouseY = mouseY;
    }

}
