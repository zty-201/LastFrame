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
#include <string>   // <-- ADDED (for std::string)
#include <map>      // <-- ADDED (for std::map)
#include <glm/glm.hpp>
#include "ISystem.hpp"
#include "Key_Action.hpp"

#ifdef PLATFORM_WINDOWS
#include "GLFW/glfw3.h"
#endif

#if defined(DEBUG)
#include "KeyMapping.hpp"
#include "event_structures.hpp"
#endif

#ifdef PLATFORM_ANDROID
#include <android/native_activity.h>
#include <android/input.h>
#endif

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
        static inline InputManager* this_class = nullptr;

//#if defined(PLATFORM_WINDOWS) //these keys are windows only
        // --- Keyboard API ---
        static bool GetKey(int key);       // true while held
        static bool GetKeyDown(int key);   // true on press
        static bool GetKeyUp(int key);     // true on release

        static bool GetKey(Key_Actions action);       // true while held
        static bool GetKeyDown(Key_Actions action);   // true on press
        static bool GetKeyUp(Key_Actions action);     // true on release

        // --- NEW: String-based action API (editor key mapping -> gameplay) ---
        static bool GetAction(const std::string& action);       // true while held
        static bool GetActionDown(const std::string& action);   // true on press
        static bool GetActionUp(const std::string& action);     // true on release

        static bool GetAction(int action);       // true while held
        static bool GetActionDown(int action);   // true on press
        static bool GetActionUp(int action);     // true on release

        static void SetCursorLocked(bool locked);       // hide cursor in-game
        static void SetCursorVisible(bool visible);     // hide cursor in-game 
//#endif

        // --- Mouse API ---
        static bool GetMouseButton(int button);
        static bool GetMouseButtonDown(int button);
        static bool GetMouseButtonUp(int button);

        static std::pair<double, double> GetMousePosition();
        static std::pair<double, double> GetMouseDelta();
        static std::pair<double, double> GetScrollDelta();
        static std::pair<double, double> GetTouchPosition(int pointerId);

        // --- Window events ---
        static std::pair<int, int> GetWindowSize();

        //Unified function to get device screen size
        static std::pair<int, int> GetScreenSize();
#ifdef PLATFORM_WINDOWS
        static void RegisterCallbacks(GLFWwindow* window);
#endif

        const std::unordered_map<int, KeyState>& GetKeyStates() const { return keyStates; }
        const std::unordered_map<int, KeyState>& GetMouseButtonStates() const { return mouseButtonStates; }


#ifdef PLATFORM_ANDROID
        static void RegisterCallbacks(struct android_app* app);
        static void UpdateGyroscope(float x, float y, float z, long long timestamp);

        static void UpdateKey(int keyCode, bool isDown);
        static void UpdateTouch(int action, int pointerId, float x, float y);

        //Mutators for JNI
        static void SetWindowWidth(int width);
        static void SetWindowHeight(int height);
#endif

        static std::unordered_map<int, KeyState> keyStates;

        static glm::vec3 GetGyroscopeRotationRate();
        static std::unordered_map<Key_Actions, int> key_mapping;
        static std::map <std::string, int> key_mapping_data; //json file data
        void codegen_deseralise();
        void initialise_keymapping();
        void seralise(); //coding in release build (might change the structure in src files too, so better to keep it here)

#if defined(DEBUG) && defined(PLATFORM_WINDOWS)     
        //saving of new keys only happens in debug - level editor
        void key_mapping_codegen_script(); //avoid using it FOR NOW (I am still testing)
        void key_mapping_codegen(); //write to initialise_keymapping
#endif
        

    private:
#ifdef PLATFORM_WINDOWS
        // --- GLFW Callbacks ---
        static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
        static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
        static void CursorPosCallback(GLFWwindow* window, double xpos, double ypos);
        static void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
        static void WindowSizeCallback(GLFWwindow* window, int width, int height);
        static inline GLFWwindow* s_Window = nullptr;
#endif
        static void ErrorCallback(int error, const char* description);

        // --- State storage ---
        static std::unordered_map<int, KeyState> mouseButtonStates;
        static std::unordered_map<int, std::pair<double, double>> touchPositions;

        static double mouseX, mouseY;
        static double lastMouseX, lastMouseY;
        static double scrollX, scrollY;
        static double offset_MouseX, offset_MouseY;
        static int windowWidth, windowHeight;

        static glm::vec3 currentGyroRate; //Store angular velocity (rad/s)
        static long long lastGyroTimestamp;


#if defined(DEBUG) && defined(PLATFORM_WINDOWS)

        public:

        bool editor_simulation_running;
        bool editor_running;

    
        void HANDLE_EVENT_LevelEditor_Simulation(Event_Level_Editor_Simulation& play)
        {
            //Only draw the scene to the framebuffer if in level editor.
            editor_simulation_running = play.is_enabled;
        }

        void HANDLE_EVENT_LevelEditorToggled(Event_Level_Editor_Toggled& editor_toggled)
        {
            //Only draw the scene to the framebuffer if in level editor.
            editor_running = editor_toggled.is_enabled;
        }
#endif

    private:
        bool LoadKeyMappingFromJson();
        void PopulateDefaultKeyMappingData();


    };

}

#endif
