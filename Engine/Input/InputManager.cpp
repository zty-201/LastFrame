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


#if defined(DEBUG) && defined(PLATFORM_WINDOWS)
#include "System_LevelEditor.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>

#endif

#include "AssetManager.hpp"

// Define constants from Java MotionEvent for direct use in C++
// These are standard Android MotionEvent action values
#define ACTION_DOWN         0
#define ACTION_UP           1
#define ACTION_MOVE         2
#define ACTION_CANCEL       3
#define ACTION_POINTER_DOWN 5
#define ACTION_POINTER_UP   6

namespace EngineFramework
{

    // Static member definitions
    std::unordered_map<int, KeyState> InputManager::keyStates;
    std::unordered_map<Key_Actions, int> InputManager::key_mapping;


//#if defined(DEBUG) && defined(PLATFORM_WINDOWS)
    std::map <std::string, int> InputManager::key_mapping_data;
//#endif   

    std::unordered_map<int, KeyState> InputManager::mouseButtonStates;

    double InputManager::mouseX = 0.0;
    double InputManager::mouseY = 0.0;
    double InputManager::lastMouseX = 0.0;
    double InputManager::lastMouseY = 0.0;

    double InputManager::scrollX = 0.0;
    double InputManager::scrollY = 0.0;

    double InputManager::offset_MouseX = 0.0;
    double InputManager::offset_MouseY = 0.0;

    int InputManager::windowWidth = 0;
    int InputManager::windowHeight = 0;

    glm::vec3 InputManager::currentGyroRate{};
    long long InputManager::lastGyroTimestamp{};

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

        //auto& pScene = GraphicsSystem::GetInstance().GetScene();
        //std::shared_ptr<ShaderProgram> shader = EngineFramework::AssetManagerSystem::GetResource<ShaderProgram>(pScene->shader_guid);
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
            mouseButtonStates[button] = KeyState::Pressed;
        }
        else if (action == GLFW_RELEASE) { mouseButtonStates[button] = KeyState::Released; }
    }

    void InputManager::CursorPosCallback(GLFWwindow* window, double xpos, double ypos)
    {
        UNREFERENCED_PARAMETER(window);

        // UPDATE ABSOLUTE MOUSE POSITION
        mouseX = xpos;
        mouseY = ypos;

        // UPDATE FRAME-TO-FRAME DELTA
        offset_MouseX = xpos - lastMouseX;
        offset_MouseY = ypos - lastMouseY;

        lastMouseX = xpos;
        lastMouseY = ypos;

        // Handle editor / gameplay camera controls
        if (mouseButtonStates[GLFW_MOUSE_BUTTON_LEFT] == KeyState::Pressed
            || mouseButtonStates[GLFW_MOUSE_BUTTON_LEFT] == KeyState::Down)
        {
#if defined(DEBUG) && defined(PLATFORM_WINDOWS)
            // Editor -- do nothing (editor handles it)
#else
            auto camera_view = registry.view<CameraComponent>();
            auto& camera = camera_view.get<CameraComponent>(camera_view.front());
            Camera::onCursor(camera, offset_MouseX, offset_MouseY);
#endif
        }
        else if (mouseButtonStates[GLFW_MOUSE_BUTTON_RIGHT] == KeyState::Pressed
            || mouseButtonStates[GLFW_MOUSE_BUTTON_RIGHT] == KeyState::Down)
        {
#if defined(DEBUG) && defined(PLATFORM_WINDOWS)
            // Editor -- do nothing
#else
            auto light_view = registry.view<LightComponent>();
            auto& light = light_view.get<LightComponent>(light_view.front());
            Light::onCursor(light, offset_MouseX, offset_MouseY);
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
#ifdef PLATFORM_ANDROID
    void InputManager::RegisterCallbacks(struct android_app* app)
    {
        (void)app;
    }

    void InputManager::UpdateKey(int keyCode, bool isDown)
    {
        //LOGI("Android Key Input: Code=%d, Action=%s", keyCode, isDown ? "DOWN" : "UP");
        if (isDown)
        {
            keyStates[keyCode] = KeyState::Pressed;
        }
        else
        {
            keyStates[keyCode] = KeyState::Released;
        }
    }

    void InputManager::UpdateTouch(int action, float x, float y)
    {
        mouseX = (double)x;
        mouseY = (double)y;

        constexpr int PRIMARY_BUTTON = 0; //Maps to GLFW_MOUSE_BUTTON_LEFT

        //LOGI("Android Touch Input: Action=%d, Pos=(%.1f, %.1f)", action, x, y);

        switch (action)
        {
        case ACTION_DOWN:
        case ACTION_POINTER_DOWN:
            mouseButtonStates[PRIMARY_BUTTON] = KeyState::Pressed;
            break;

        case ACTION_UP:
        case ACTION_POINTER_UP:
        case ACTION_CANCEL:
            mouseButtonStates[PRIMARY_BUTTON] = KeyState::Released;
            break;

        case ACTION_MOVE:
            if (mouseButtonStates[PRIMARY_BUTTON] == KeyState::Pressed)
            {
                mouseButtonStates[PRIMARY_BUTTON] = KeyState::Down;
            }
            break;
        }
    }

    void InputManager::SetWindowWidth(int width)
    {
        windowWidth = width;
    }

    void InputManager::SetWindowHeight(int height)
    {
        windowHeight = height;
    }

    void InputManager::UpdateGyroscope(float x, float y, float z, long long timestamp)
    {
        //Android provides angular velocity in rad/s
        currentGyroRate.x = x;
        currentGyroRate.y = y;
        currentGyroRate.z = z;
        lastGyroTimestamp = timestamp;
        //LOGI("Gyro Update: X=%.4f, Y=%.4f, Z=%.4f", x, y, z);
    }
#endif
    void InputManager::ErrorCallback(int error, const char* description)
    {
        (void)error;
        LOGE("GLFW Error: %s", description);
    }

#if defined(DEBUG) && defined(PLATFORM_WINDOWS)

    bool InputManager::GetAction(const std::string& action)
    {
        if (!(InputManager::GetInstance().editor_simulation_running || !InputManager::GetInstance().editor_running))
            return false;

        auto it = key_mapping_data.find(action);
        if (it == key_mapping_data.end()) return false;
        return GetKey(it->second);
    }

    bool InputManager::GetActionDown(const std::string& action)
    {
        if (!(InputManager::GetInstance().editor_simulation_running || !InputManager::GetInstance().editor_running))
            return false;

        auto it = key_mapping_data.find(action);
        if (it == key_mapping_data.end()) return false;
        return GetKeyDown(it->second);
    }

    bool InputManager::GetActionUp(const std::string& action)
    {
        if (!(InputManager::GetInstance().editor_simulation_running || !InputManager::GetInstance().editor_running))
            return false;

        auto it = key_mapping_data.find(action);
        if (it == key_mapping_data.end()) return false;
        return GetKeyUp(it->second);
    }

    //KEY ACTION ENUM CALLING

    bool InputManager::GetAction(int action)
    {
        if (!(InputManager::GetInstance().editor_simulation_running || !InputManager::GetInstance().editor_running))
            return false;

        auto it = key_mapping.find(static_cast<EngineFramework::Key_Actions>(action));
        if (it == key_mapping.end()) return false;
        return GetKey(it->second);
    }

    bool InputManager::GetActionDown(int action)
    {
        if (!(InputManager::GetInstance().editor_simulation_running || !InputManager::GetInstance().editor_running))
            return false;

        auto it = key_mapping.find(static_cast<EngineFramework::Key_Actions>(action));
        if (it == key_mapping.end()) return false;
        return GetKeyDown(it->second);
    }

    bool InputManager::GetActionUp(int action)
    {
        if (!(InputManager::GetInstance().editor_simulation_running || !InputManager::GetInstance().editor_running))
            return false;

        auto it = key_mapping.find(static_cast<EngineFramework::Key_Actions>(action));
        if (it == key_mapping.end()) return false;
        return GetKeyUp(it->second);
    }

    





#endif

    bool InputManager::GetKey(int key)
    {
#if defined(DEBUG) && defined(PLATFORM_WINDOWS)   

        if (!(InputManager::GetInstance().editor_simulation_running || !InputManager::GetInstance().editor_running)) {
            return false;
        }
#endif

        auto it = keyStates.find(key);
        return it != keyStates.end() && (it->second == KeyState::Pressed || it->second == KeyState::Down);

    }

    bool InputManager::GetKeyDown(int key)
    {
#if defined(DEBUG) && defined(PLATFORM_WINDOWS)   

        if (!(InputManager::GetInstance().editor_simulation_running || !InputManager::GetInstance().editor_running)) {
            return false;
        }



#endif
        auto it = keyStates.find(key);
        return it != keyStates.end() && it->second == KeyState::Pressed;
    }

    bool InputManager::GetKeyUp(int key)
    {
#if defined(DEBUG) && defined(PLATFORM_WINDOWS)   

        if (!(InputManager::GetInstance().editor_simulation_running || !InputManager::GetInstance().editor_running)) {
            return false;
        }
#endif
        auto it = keyStates.find(key);
        return it != keyStates.end() && it->second == KeyState::Released;
    }
    /*
        FOR KEY MAPPING
    */

    bool InputManager::GetKey(Key_Actions action) {

#if defined(DEBUG) && defined(PLATFORM_WINDOWS)   

        if (!(InputManager::GetInstance().editor_simulation_running || !InputManager::GetInstance().editor_running)) {
            return false;
        }
#endif

        auto it = key_mapping.find(action);
        return it != key_mapping.end() && GetKey(it->second);

    }
    bool InputManager::GetKeyDown(Key_Actions action) {
#if defined(DEBUG) && defined(PLATFORM_WINDOWS)   

        if (!(InputManager::GetInstance().editor_simulation_running || !InputManager::GetInstance().editor_running)) {
            return false;
        }
#endif

        auto it = key_mapping.find(action);
        return it != key_mapping.end() && GetKeyDown(it->second);

    }
    bool InputManager::GetKeyUp(Key_Actions action) {

#if defined(DEBUG) && defined(PLATFORM_WINDOWS)   

        if (!(InputManager::GetInstance().editor_simulation_running || !InputManager::GetInstance().editor_running)) {
            return false;
        }
#endif

        auto it = key_mapping.find(action);
        return it != key_mapping.end() && GetKeyUp(it->second);

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
        return { offset_MouseX, offset_MouseY };
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
        initialise_keymapping();



#if defined(DEBUG) && defined(PLATFORM_WINDOWS)   

        event_dispatcher.sink<Event_Level_Editor_Simulation>().connect<&InputManager::HANDLE_EVENT_LevelEditor_Simulation>(this);
        event_dispatcher.sink<Event_Level_Editor_Toggled>().connect<&InputManager::HANDLE_EVENT_LevelEditorToggled>(this);
        codegen_deseralise();
        seralise();
#endif        
    }

    void InputManager::Update(float deltaTime)
    {
        (void)deltaTime;

        /*
            Level Editor Inputs and Core Engine inputs are seperated.

        */
#if defined(DEBUG) && defined(PLATFORM_WINDOWS)   

        if (!editor_running && !editor_simulation_running) {  // Changed condition


            if (mouseButtonStates[GLFW_MOUSE_BUTTON_LEFT] == KeyState::Pressed || (mouseButtonStates[GLFW_MOUSE_BUTTON_LEFT] == KeyState::Down)) {

                //std::cout << "left pressed\n";

                auto camera_view = registry.view<CameraComponent>();
                auto& camera = camera_view.get<CameraComponent>(camera_view.front());
                Camera::onCursor(camera, offset_MouseX, offset_MouseY);
            }

            if (GetKeyDown(GLFW_KEY_W)) {
                auto camera_view = registry.view<CameraComponent>();
                auto& camera = camera_view.get<CameraComponent>(camera_view.front());
                Camera::setType(camera, CameraComponent::CameraType::WALKING);
            }

            else if (GetKeyDown(GLFW_KEY_O)) {
                auto camera_view = registry.view<CameraComponent>();
                auto& camera = camera_view.get<CameraComponent>(camera_view.front());
                Camera::setType(camera, CameraComponent::CameraType::ORBITING);
            }

            else if (GetKeyDown(GLFW_KEY_1))
            {
                //std::cout << "1 is clicked\n";
                //shader->use();
                //shader->setUniform("materialType", 0); // PBR
            }
            else if (GetKeyDown(GLFW_KEY_2))
            {
                //shader->use();
                //shader->setUniform("materialType", 1); // Blinn-Phong
            }
        }
#else

        /*
            FOR GAMEPLAY IF NEEDED
        */

        /*if (GetKeyDown(GLFW_KEY_W)) {
            auto camera_view = registry.view<CameraComponent>();
            auto& camera = camera_view.get<CameraComponent>(camera_view.front());
            Camera::setType(camera, CameraComponent::CameraType::WALKING);
        }

        else if (GetKeyDown(GLFW_KEY_O)) {
            auto camera_view = registry.view<CameraComponent>();
            auto& camera = camera_view.get<CameraComponent>(camera_view.front());
            Camera::setType(camera, CameraComponent::CameraType::ORBITING);
        }*/

        //else if (GetKeyDown(GLFW_KEY_1))
        //{
        //    //shader->use();
        //    //shader->setUniform("materialType", 0); // PBR
        //}
        //else if (GetKeyDown(GLFW_KEY_2))
        //{
        //    //shader->use();
        //    //shader->setUniform("materialType", 1); // Blinn-Phong
        //}


#endif





//#ifdef PLATFORM_ANDROID
//        // Note: You must ensure your LOGI macro is defined/accessible
//
//        // 1. Get current position 
//        std::pair<double, double> pos = GetMousePosition();
//
//        // 2. Get delta
//        std::pair<double, double> delta = GetMouseDelta();
//
//        // Test GetMouseButtonDown (single tap detection)
//        if (GetMouseButton(0)) {
//            LOGI("TOUCH_DEBUG: SINGLE TAP DETECTED!");
//
//            // Logging the results
//            LOGI("TOUCH_DEBUG: POS=(X=%.1f, Y=%.1f) | DELTA=(dX=%.1f, dY=%.1f)",
//                pos.first, pos.second, delta.first, delta.second);
//        }
//#endif

    }

    void InputManager::PostUpdate(float deltaTime)
    {
        (void)deltaTime;

        // --- Keyboard ---
        for (auto& [key, state] : keyStates)
        {
            KeyState oldState = state; // Store the old state for debugging
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
            if (oldState != state) {
                std::cout << "[InputManager::PostUpdate] Key State Advanced: key=" << key << " " << (int)oldState << " -> " << (int)state << "\n";
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
        offset_MouseX = 0;
        offset_MouseY = 0;
        // Update last mouse position
        lastMouseX = mouseX;
        lastMouseY = mouseY;
    }

    glm::vec3 InputManager::GetGyroscopeRotatioRate()
    {
        return currentGyroRate;
    }


#if defined(DEBUG) && defined(PLATFORM_WINDOWS)
    /*ONLY AVAILABLE IN DEBUG WHERE LEVEL EDITOR IS OPENED*/


    void InputManager::seralise() {
        //STORE DATA FROM KEY_MAPPING_DATA TO JSON FILE
        //logging purpose, does not key data
        nlohmann::json data; // root JSON object
        data["gameplay_keys"] = nlohmann::json::array();

        // Loop through your map and build JSON entries
        for (const auto& [key_action, glfw_key] : key_mapping_data)
        {
            nlohmann::json entry;
            entry["key_action"] = key_action;
            entry["glfw_key"] = glfw_key;

            data["gameplay_keys"].push_back(entry);
        }

        // Write to file
        std::string path = AssetManagerSystem::GetAssetFolderPrefix().string() + "Extern/Input/key_mapping_data.json";

        std::ofstream file(path);
        if (!file.is_open())
        {
            LOGE("Failed to open file for writing: %s", path.c_str());
            return;
        }

        file << data.dump(4);
        file.close();

    }

    void InputManager::key_mapping_codegen() {

        //write to initialise_keymapping
        //write to keyaction.hpp

        std::string path = AssetManagerSystem::GetAssetFolderPrefix().string() + "Engine/Input/InputManager_codegen.cpp";

        std::ofstream file(path);
        if (!file.is_open())
        {
            LOGE("Failed to open file for writing: %s", path.c_str());
            return;
        }
        file << "#include \"InputManager.hpp\"";
        file << "\nnamespace EngineFramework\n{\n\tvoid InputManager::initialise_keymapping() {";

        for (auto const& KEYS : key_mapping_data)
        {
            std::stringstream ss;
            //key_mapping[Key_Actions::MOVE_LEFT] = GLFW_KEY_A;
            ss << "\n\t\tkey_mapping[Key_Actions::" << KEYS.first << "] = " << KEYS.second << ";";
            file << ss.str();
        }
        file << "\n\t}\n}\n";

        file.close();


        path = AssetManagerSystem::GetAssetFolderPrefix().string() + "Engine/Input/Key_Action.hpp";

        std::ofstream file_2(path);
        if (!file_2.is_open())
        {
            LOGE("Failed to open file for writing: %s", path.c_str());
            return;
        }
        file_2 << "\n#ifndef KEY_ACTION_H";
        file_2 << "\n#define KEY_ACTION_H";
        file_2 << "\nnamespace EngineFramework\n{\n\tenum class Key_Actions{";

        for (auto const& KEYS : key_mapping_data)
        {
            std::stringstream ss;
            ss << "\n\t\t" << KEYS.first << ",";
            file_2 << ss.str();
        }
        file_2 << "\n\t};\n}";
        file_2 << "\n#endif\n";

        file_2.close();

    }

    void InputManager::key_mapping_codegen_script() {

        
        std::string path = AssetManagerSystem::GetAssetFolderPrefix().string() + "Extern/Scripts/A1.txt";

        std::ofstream file_2(path);
        if (!file_2.is_open())
        {
            LOGE("Failed to open file for writing: %s", path.c_str());
            return;
        }

        if (key_mapping.empty() || key_mapping_data.empty())return;


        file_2 << "\n-- ============================================================\n";
        file_2 << "--Key_Actions.lua(Inputs)\n";
        file_2 << "-- ============================================================\n";
   
        file_2 << "\nKey_Actions = {\n";

        for (const auto& [action, keycode] : key_mapping_data)
        {
            for (const auto& [action2, keycode2] : key_mapping)
            {
                if (keycode == keycode2) {
                    std::stringstream ss;
                    ss << "\n\t" << action << " = " << (int)action2 << ",";
                    file_2 << ss.str();
                    break;
                }
            }
        }

       
        file_2 << "\n}\n";
        file_2.close();

    }


#endif

    void InputManager::codegen_deseralise() {
        //deseralise keyaction.hpp
        //checking to be more safe

        std::vector<std::string> enums;

        {
            std::string path = AssetManagerSystem::GetAssetFolderPrefix().string() + "Engine/Input/Key_Action.hpp";

            std::ifstream in_file(path);
            if (!in_file.is_open())
            {
                LOGE("Failed to open file for reading: %s", path.c_str());
                return;
            }

            // Read the entire file into a string
            std::stringstream buffer;
            buffer << in_file.rdbuf();
            std::string content = buffer.str();
            in_file.close();

            // Remove all whitespace (spaces, tabs, newlines, etc.)
            content.erase(
                std::remove_if(content.begin(), content.end(),
                    [](unsigned char c) { return std::isspace(c); }),
                content.end()
            );

            std::string start_token = "enumclassKey_Actions{";
            std::string end_token = "};";

            size_t start_pos = content.find(start_token);
            size_t end_pos = content.find(end_token, start_pos);

            if (start_pos != std::string::npos && end_pos != std::string::npos)
            {
                start_pos += start_token.length();
                std::string enum_block = content.substr(start_pos, end_pos - start_pos);

                // Split by commas
                std::stringstream ss(enum_block);
                std::string item;
                while (std::getline(ss, item, ','))
                {
                    if (!item.empty())
                        enums.push_back(item);
                }
            }

            // Print results
            /*std::cout << "Extracted enum tags:\n";
            for (const auto& tag : enums)
                std::cout << tag << std::endl;*/


        }



        //=========================================================================================================
        //deseralise codegen to keymapping data

        {
            std::string path = AssetManagerSystem::GetAssetFolderPrefix().string() + "Engine/Input/InputManager_codegen.cpp";

            std::ifstream in_file2(path);
            if (!in_file2.is_open())
            {
                LOGE("Failed to open file for reading: %s", path.c_str());
                return;
            }

            // Read the entire file into a string
            std::stringstream buffer2;
            buffer2 << in_file2.rdbuf();
            std::string content = buffer2.str();
            in_file2.close();

            // Remove all whitespace (spaces, tabs, newlines, etc.)
            content.erase(
                std::remove_if(content.begin(), content.end(),
                    [](unsigned char c) { return std::isspace(c); }),
                content.end()
            );

            for (const auto& tag : enums)
            {
                // Find Key_Actions::<tag>
                std::string enum_tag = "Key_Actions::" + tag;
                size_t pos = content.find(enum_tag);
                if (pos != std::string::npos)
                {
                    //2 cases
                    //1: key_mapping[Key_Actions::JUMP] = 74;
                    //2: key_mapping[Key_Actions::JUMP] = 74, key_mapping[Key_Actions::RUN] = 73;
                    //Pick the nearest next pos of either
                    size_t equal_pos = content.find('=', pos);

                    if (equal_pos != std::string::npos)
                    {
                        size_t semi_pos = content.find(';', equal_pos);
                        size_t comma_pos = content.find(',', equal_pos);
                        size_t end_pos = 0;

                        //this is rather impossible, if so, ur program cant compile :/
                        if (semi_pos == std::string::npos && comma_pos == std::string::npos) {
                            LOGE("No valid end found for key mapping assignment.");
                            continue;
                        }

                        // if ',' exists and it comes before ';'
                        if (comma_pos != std::string::npos && comma_pos < semi_pos) {
                            end_pos = comma_pos;
                        }
                        else {
                            end_pos = semi_pos;
                        }

                        if (end_pos != std::string::npos)
                        {
                            std::string number_str = content.substr(equal_pos + 1, end_pos - equal_pos - 1);
                            if (!number_str.empty())
                                key_mapping_data[tag] = std::stoi(number_str);
                        }
                    }
                }
            }


            /*for (const auto& [key, value] : key_mapping_data)
                std::cout << key << " = " << value << std::endl;*/

        }

    }



}
