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
#ifndef UI_SYSTEM_HPP
#define UI_SYSTEM_HPP

#include <unordered_map>
#include <string>   // <-- ADDED (for std::string)
#include <map>      // <-- ADDED (for std::map)
#include <glm/glm.hpp>
#include "ISystem.hpp"
#include "ButtonComponent.hpp"

#ifdef PLATFORM_WINDOWS
#include "GLFW/glfw3.h"
#endif

#if defined(DEBUG)

#endif

#ifdef PLATFORM_ANDROID
#include <android/native_activity.h>
#include <android/input.h>
#endif

namespace EngineFramework
{
    //menu/overlay contains:
    //1. buttons(clickable)
    //2. just texture in ndc


    /*
    * UI MENU CLASS MANAGES ELEMENTS WITH OVERLAY_COMPONENT
    * THIS INCLUDES BUTTONS AND NON-BUTTONS 
    */

    //enable for window debug first (avoid android cause android now got issue)
#if defined(DEBUG) && defined(PLATFORM_WINDOWS)

    class UI_Menu {

    public:

        std::string menu_name;
        void Initialize();
        void Update();
        void Clean();
        void Seralise(); //Writing to files
        void Deseralise(); //Reading from files
        void Add_Element(entt::entity e);
        void Remove_Element(entt::entity e);

        bool is_menu_active;

    private:

        static std::vector<entt::entity> AllElements_in_this_overlay; //entities with overlaycomponent
        void update_Buttons();
        void update_Button_State(); //update the button state based on input_manager
        void Trigger_Button_Event();

        void draw_elements(); //draw if overlay is active

    };


    class UI_System : public EngineFramework::ISystem
    {
    public:
        //ISystem implementation
        void Initialize() override;
        void Update(float deltaTime) override;      // promote transitions
        void PostUpdate(float deltaTime) override;  // finalize states

        UI_System() { this_class = this; }
        static UI_System& GetInstance() { return *this_class; }
        static inline UI_System* this_class = nullptr;


        static std::unordered_map<std::string, UI_Menu> Menu_Mapping;
        std::string current_menu;


    private:

        static void ErrorCallback(int error, const char* description);
        void activate_current_menu();

    };
#endif

}
#endif

