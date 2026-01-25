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
#include "UISystem.hpp"
#include "GraphicsSystem.h"


#if defined(DEBUG) && defined(PLATFORM_WINDOWS)
#include "System_LevelEditor.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>

#endif

#include "AssetManager.hpp"

namespace EngineFramework
{

#if defined(DEBUG) && defined(PLATFORM_WINDOWS)
    std::unordered_map<std::string, UI_Menu> UI_System::Menu_Mapping;

    void UI_System::ErrorCallback(int error, const char* description)
    {
        (void)error;
        UNREFERENCED_PARAMETER(description);
        //LOGE("GLFW Error: %s", description);
    }

    void UI_System::Initialize()
    {
        std::cout << "=========================================\n";
        std::cout << "        UI_System::Initialize()          \n";
        std::cout << "=========================================\n";


#if defined(DEBUG) && defined(PLATFORM_WINDOWS)   

        //event_dispatcher.sink<Event_Level_Editor_Simulation>().connect<&InputManager::HANDLE_EVENT_LevelEditor_Simulation>(this);
        //event_dispatcher.sink<Event_Level_Editor_Toggled>().connect<&InputManager::HANDLE_EVENT_LevelEditorToggled>(this);
#endif
      
    }

    void UI_System::Update(float deltaTime)
    {
        (void)deltaTime;

        /*
            Level Editor Inputs and Core Engine inputs are seperated.

        */
#if defined(DEBUG) && defined(PLATFORM_WINDOWS)   

        //if (!editor_running && !editor_simulation_running) {  // Changed condition

        //}

#endif
    }

    void UI_System::PostUpdate(float deltaTime)
    {
        (void)deltaTime;

        
    }

    void UI_System::activate_current_menu() {

    }

#endif    

}
