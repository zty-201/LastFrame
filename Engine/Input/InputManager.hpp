/*
*****************************************************************/
/*!
\file   Input.hpp
\author Tang Yeun, Dean
\par    y.tang\@digipen.edu
\date   September 2025
\brief
This file contains the declarations of the functions that are needed for
handling the inputs in the system. 
Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* *******************************************************************/
#ifndef INPUT_MANAGER_H
#define INPUT_MANAGER_H

#include <unordered_map>
#include "ISystem.hpp"
#include "GLFW/glfw3.h"
namespace EngineFramework
{
    enum class KeyState
    {
        None,		//Default state, no interaction with key
        Pressed,	//Key pressed this frame
        Down,		//Key held
        Released	//Key released this frame
    };

    class InputManager : public EngineFramework::ISystem
    {
    public:
        //ISystem implementation
        void Initialize() override;
        void Update(float deltaTime) override;      // promote transitions
        void PostUpdate(float deltaTime) override;  // finalize states

        InputManager() { this_class = this; }
        static InputManager& GetInstance() { return *this_class; }
        static inline InputManager *this_class = nullptr;

        // --- Keyboard API ---
        static bool GetKey(int key);       // true while held
        static bool GetKeyDown(int key);   // true on press
        static bool GetKeyUp(int key);     // true on release

        // --- Mouse API ---
        static bool GetMouseButton(int button);
        static bool GetMouseButtonDown(int button);
        static bool GetMouseButtonUp(int button);

        static std::pair<double, double> GetMousePosition();
        static std::pair<double, double> GetMouseDelta();
        static std::pair<double, double> GetScrollDelta();

        // --- Window events ---
        static std::pair<int, int> GetWindowSize();

        static void RegisterCallbacks(GLFWwindow* window);

        const std::unordered_map<int, KeyState>& GetKeyStates() const { return keyStates; }
        const std::unordered_map<int, KeyState>& GetMouseButtonStates() const { return mouseButtonStates; }
static std::unordered_map<int, KeyState> keyStates;
    private:
        // --- GLFW Callbacks ---
        static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
        static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
        static void CursorPosCallback(GLFWwindow* window, double xpos, double ypos);
        static void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
        static void WindowSizeCallback(GLFWwindow* window, int width, int height);
        static void ErrorCallback(int error, const char* description);

        // --- State storage ---
        
        static std::unordered_map<int, KeyState> mouseButtonStates;

        static double mouseX, mouseY;
        static double lastMouseX, lastMouseY;
        static double scrollX, scrollY;
        static int windowWidth, windowHeight;
    };

}

#endif