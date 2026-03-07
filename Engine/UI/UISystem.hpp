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
#include "JoystickComponent.h"
#include "InputManager.hpp"

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
//#if defined(DEBUG) && defined(PLATFORM_WINDOWS)

    class UI_Menu {

    public:

        UI_Menu(const std::string& name) : menu_name(name) {};
        ~UI_Menu(){
            AllElements_in_this_overlay.clear();
        };
        std::string menu_name;
        void Initialize();
        void Update();
        void Clean();

   
        void removeAllEntities_Reg();
        bool Deseralise(std::string); //Reading from files
#ifdef PLATFORM_WINDOWS  //Not used on android.
        std::unordered_set<entt::entity> Seralise();
#endif
        void Add_Element(entt::entity e);
        void Remove_Element(entt::entity e);

        bool is_menu_active;
        void draw_elements(bool value = true); //draw if overlay is active
        std::vector<entt::entity> get_Elements(){
            return AllElements_in_this_overlay;
        };

    private:

        std::vector<entt::entity> AllElements_in_this_overlay; //entities with overlaycomponent
        void update_Buttons();
        void update_Button_State(ButtonComponent& button, entt::entity e); //update the button state based on input_manager
        void Trigger_Button_Event(ButtonComponent& button, BUTTON_STATE state);
        bool check_touch(entt::entity e);
       

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

        //after testing uibuttons first
        static std::unordered_map<std::string, std::pair<std::unique_ptr<UI_Menu>,bool>> Menu_Mapping;
        std::string current_menu;

        void Clean_All();
        void DeserializeAllOverlays();
        bool DeserializeOverlay(std::string);

        void displayMenu(std::string menu);
        void closeMenu(); //this is current menu
        void closeMenu(std::string menu);
        void closeAllMenu();
        std::vector<entt::entity> getMenu_Entities(std::string menu);
        std::unordered_set<entt::entity> getAllMenu_Entities();

        void update_Joysticks();
        
#ifdef PLATFORM_WINDOWS  //Not used on android.
        std::unordered_set<entt::entity> Seralise_AllMenus();
        void create_overlay(std::string menu);
#endif

        void add_entity_fromMenu(std::string menu, entt::entity e);
        void remove_entity_fromMenu(std::string menu, entt::entity e);

        void Load_Start_Menu();

        // Cutscene controls
        void StartPseudoCutscene();
        void StartCutsceneSequence(const std::vector<std::string>& sceneMenus,
            float intervalSeconds = 1.5f,
            bool closeOthersFirst = true);
        void StopCutsceneSequence(bool closeAll = true);
        bool IsCutscenePlaying() const;


    private:

        static void ErrorCallback(int error, const char* description);
        

    };
//#endif

}
#endif

