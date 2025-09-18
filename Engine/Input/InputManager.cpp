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

        switch (action)
        {
        case GLFW_PRESS:
            // Transition to Pressed (this frame only)
            keyStates[key] = KeyState::Pressed;
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
            std::cout << "Mouse button pressed: button = " << button << std::endl;
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

        auto& pScene = g_graphics_ptr->GetScene();
        //std::cout << "CursorPosCallback: xoffset = " << xoffset << ", yoffset = " << yoffset << std::endl;
        if (mouseButtonStates[GLFW_MOUSE_BUTTON_LEFT] == KeyState::Pressed 
            || (mouseButtonStates[GLFW_MOUSE_BUTTON_LEFT] == KeyState::Down))
        {
			std::cout << "CursorPosCallback: xoffset = " << xoffset << ", yoffset = " << yoffset << std::endl;
            pScene->camerasOnCursor(xoffset, yoffset);
        }
        else if (mouseButtonStates[GLFW_MOUSE_BUTTON_RIGHT] == KeyState::Pressed)
        {
            pScene->lightsOnCursor(xoffset, yoffset);
        }
    }

    void InputManager::ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
    {
        (void)window;
        scrollX = xoffset;
        scrollY = yoffset;
    }

    void InputManager::WindowSizeCallback(GLFWwindow* window, int width, int height)
    {
        (void)window;
        windowWidth = width;
        windowHeight = height;
    }
#endif
    void InputManager::ErrorCallback(int error, const char* description)
    {
        (void)error;
        fprintf(stderr, "GLFW Error: %s\n", description);
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
        //Do nothing
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
