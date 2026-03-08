/*
*****************************************************************/
/*!
\file   UIMenu.cpp
\author Tan Yee Tong
\par    yeetong.t\@digipen.edu
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
#include "System_GameObjectSystem.hpp"
#include "UIComponent.h"
#include "InputManager.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include "AssetManager.hpp"
#include "ScriptWrapper.hpp"

#if defined(DEBUG) && defined(PLATFORM_WINDOWS)
#include "System_LevelEditor.hpp"
#include "Screen.hpp"
#endif



namespace EngineFramework
{
//#if defined(DEBUG) && defined(PLATFORM_WINDOWS)
    
    void UI_Menu::Initialize() {

        //menu_name = "Menu_1";

        //std::cout << "=========================================\n";
        //std::cout << "Initialize(): Menu Name = " << menu_name << std::endl;

        std::vector <entt::entity> to_remove;

        for (auto entity : AllElements_in_this_overlay) {        

            /*
                EXTRA FILTERING CHECKS
            */

            if (!registry.valid(entity)) {
                //std::cout << "!registry.valid(entity) \n";
                to_remove.emplace_back(entity);
                continue;
            }

            /*
                EXTRA FILTERING CHECKS
            */
            if (registry.all_of<UIComponent>(entity)) {
                auto& ui = registry.get<UIComponent>(entity);

                if (menu_name != ui.Overlay_name) {
                    //std::cout << "menu_name != ui.Overlay_name \n";
                    //std::cout << "menu_name = " << menu_name << std::endl;
                    //std::cout << "ui.Overlay_name = " << ui.Overlay_name << std::endl;
                    
                    to_remove.emplace_back(entity);
                    continue;
                }

                ui.belong_to_menu = true;
                ui.visible = false; //set it to false first. /* !!!!!!!!!!!!!!!!!!!!!!!!! */
               
            }

            if (registry.all_of<ButtonComponent>(entity)) {
                auto& button = registry.get<ButtonComponent>(entity);
                                   
                button.is_hover = false;
                button.is_clicked = false;
                button.is_released = false; 
                //button.is_active = false;

                button.curr_button_state = IDLE;
                button.prev_button_state = IDLE;
            }
        }

        if (!to_remove.empty()) {

            //std::cout << "I AM CLEARING THE RUBISH ENTITY\n";

            AllElements_in_this_overlay.erase(

                std::remove_if(AllElements_in_this_overlay.begin(),AllElements_in_this_overlay.end(),[&](entt::entity e) {
                        return std::find(to_remove.begin(),to_remove.end(),e) != to_remove.end();
                    }
                ),

                AllElements_in_this_overlay.end()
            );

            to_remove.clear();

        }
        
            
        //std::cout << "UI_Menu::Initialize() | SIZE: " << AllElements_in_this_overlay.size() <<std::endl; //should be 1 for now
        //std::cout << "=========================================\n";

    }

    void UI_Menu::Update() {

        update_Buttons();
    }

    void UI_Menu::Clean() {
        AllElements_in_this_overlay.clear();
    }
    
    bool UI_Menu::Deseralise(std::string filename) {

        
        //menu_name = filename;

        if (filename.empty()) return false;

        AllElements_in_this_overlay.clear();
        
        std::string sceneString = AssetManagerSystem::GetStringFromFile("Extern/Overlays/" + filename + ".json");
        GameObjectSystem::GetInstance().DeSerializeAllEntityInstances(sceneString, AllElements_in_this_overlay);

        // [BEN] Testing if buttons work now
        for (auto e : AllElements_in_this_overlay)
            Engine::ScriptSystem::RequestReloadEntity(e);


        //std::cout << "=========================================\n";
        //std::cout << "Deseralise(): Menu Name = " << menu_name << std::endl;

        //for (auto entity : AllElements_in_this_overlay) {

        //    if (registry.all_of<UIComponent>(entity)) {       

        //        /*
        //            SWAPPING OF TEXTURES FOR UI COMPONENT
        //        */
        //        auto& ui = registry.get<UIComponent>(entity);
        //        //std::cout << "entity: " << (int)entity << " | ui.Overlay_name =" << ui.Overlay_name << std::endl;
        //    
        //    }
   
        //}

        //std::cout << "=========================================\n";

        
        
        return !AllElements_in_this_overlay.empty();

    }

    
#ifdef PLATFORM_WINDOWS
    std::unordered_set<entt::entity> UI_Menu::Seralise() {



        if (menu_name.empty()) return {};

        std::string path = AssetManagerSystem::GetAssetFolderPrefix().string() + "Extern/Overlays/" + menu_name + ".json";
        std::ofstream Out_File(path);

        if (!Out_File.is_open()) return{};

        nlohmann::json combined = nlohmann::json::object();

        std::unordered_set<entt::entity> entity_seralised;
        
        for (auto entity : AllElements_in_this_overlay) {         

            if (registry.all_of<UIComponent>(entity)) {

                
                auto& ui = registry.get<UIComponent>(entity);
                if (!ui.belong_to_menu) continue;
                if (ui.Overlay_name != menu_name) continue;

                // Serialize single entity -> parse -> merge into combined
                const std::string one = GameObjectSystem::GetInstance().SerializeSingleEntity(entity);
                //registry.destroy(entity); //remove entity from the registry, so we dont write to scene file since it exists in the thing alr

                nlohmann::json oneJson = nlohmann::json::parse(one, nullptr, false);
                if (oneJson.is_discarded() || !oneJson.is_object()) continue;

                // oneJson is like { "12": {...} }, merge it into combined
                for (auto it = oneJson.begin(); it != oneJson.end(); ++it)
                {
                    combined[it.key()] = it.value();
                }

                entity_seralised.insert(entity);
            
            }

        }

        Out_File << combined.dump(1, '\t'); // Output the contents of the stringstream
        Out_File.close();

        return entity_seralised;

    }
#endif
    void UI_Menu::Add_Element(entt::entity e) {
        /*
            NEW ENTITIES WITH NEW INITIALISED VALUE
            SINCE, ENTITIES ARE ADDED TO THE MENU, IT MEANS ITS VISIBLE AH.
        */
        if (!registry.valid(e)) return; //DO NOT ADD IF ITS NOT VALID

        if (registry.all_of<UIComponent>(e)) {
            auto& ui = registry.get<UIComponent>(e);
            ui.belong_to_menu = true;
            ui.Overlay_name = menu_name;      
            ui.visible = true;
            
        }

        AllElements_in_this_overlay.emplace_back(e);
    }
    void UI_Menu::Remove_Element(entt::entity e) {

        auto it = std::remove(AllElements_in_this_overlay.begin(), AllElements_in_this_overlay.end(), e);
        AllElements_in_this_overlay.erase(it, AllElements_in_this_overlay.end());
    }

    void UI_Menu::update_Buttons() {

        for (auto entity : AllElements_in_this_overlay) {
        
            if (registry.all_of<ButtonComponent>(entity)) {
                auto& button = registry.get<ButtonComponent>(entity);
                
                update_Button_State(button, entity);

                if (registry.all_of<UIComponent>(entity)) {

                    /*
                        TRIGGERING EVENTS
                    */
                    if (button.curr_button_state == TRIGGERED) {
                        Trigger_Button_Event(button, TRIGGERED);
                        break;
                    }
                    else if (button.curr_button_state == HOVER) {
                        Trigger_Button_Event(button, HOVER);
                    }

                    /*
                        SWAPPING OF TEXTURES FOR UI COMPONENT
                    */
                    auto& ui = registry.get<UIComponent>(entity);
                    
                    if (button.prev_button_state != button.curr_button_state) {
                        if (button.curr_button_state == IDLE) {
                            ui.textureid = button.idle_texture;
                        }else if (button.curr_button_state == HOVER) {
                            ui.textureid = button.hover_texture;                          
                        }else if (button.curr_button_state == CLICKED) {
                            ui.textureid = button.clicked_texture;
                        }
                        
                    }
                }

                button.prev_button_state = button.curr_button_state;
            }
        }

    }

    void UI_Menu::update_Button_State(ButtonComponent& button, entt::entity e) {

        button.is_hover = false;
        button.is_clicked = false;
        button.is_released = false;
       
        //checked if clicked
        if (InputManager::GetInstance().GetMouseButton(GLFW_MOUSE_BUTTON_LEFT)) {

            if (check_touch(e)) {

                button.curr_button_state = CLICKED;
                button.is_clicked = true;
                //std::cout << "CLICKED!!!\n";
            }
            
        }else if (InputManager::GetInstance().GetMouseButtonUp(GLFW_MOUSE_BUTTON_LEFT)) {
            button.curr_button_state = RELEASED;  
            button.is_released = true;
            //std::cout << "RELEASED!!!\n";
        }
        else if(check_touch(e)){

#if defined(PLATFORM_WINDOWS) //ONLY WINDOWS HAVE THIS
            button.curr_button_state = HOVER;
            button.is_hover = true;
            /*std::cout << "HOVER!!!\n";*/
#endif
            
        }
        else {
            button.curr_button_state = IDLE;          
        }

        if (button.curr_button_state == RELEASED && button.prev_button_state == CLICKED) {
            button.curr_button_state = TRIGGERED;           
        }
     
    }

    bool UI_Menu::check_touch(entt::entity e) {

        if (!registry.valid(e)) return false;
        if (!registry.all_of<UIComponent>(e)) return false;

        const UIComponent& ui = registry.get<UIComponent>(e);
        if (!ui.visible) return false;

        
 //debug       
#if defined(DEBUG) && defined(PLATFORM_WINDOWS)

        glm::vec2 mouseUI{};

        if (InputManager::GetInstance().editor_running) {
            mouseUI = glm::vec2(Screen::GetInstance().GetMouseInSceneUI());
        }
        else {
            float MouseX = static_cast<float>(InputManager::GetInstance().GetMousePosition().first);
            float MouseY = static_cast<float>(InputManager::GetInstance().GetMousePosition().second);

            mouseUI = glm::vec2(MouseX, GraphicsSystem::GetInstance().Getscreen().y - MouseY);
        }

        
#endif

        //release
#if defined(NDEBUG) && defined(PLATFORM_WINDOWS)
        float MouseX = static_cast<float>(InputManager::GetInstance().GetMousePosition().first);
        float MouseY = static_cast<float>(InputManager::GetInstance().GetMousePosition().second);

        glm::vec2 mouseUI(MouseX, GraphicsSystem::GetInstance().Getscreen().y - MouseY);
#endif
     
#if defined(PLATFORM_ANDROID)
        /*
        * PLEASE CHANGE THIS ACCORDINGLY TO ANDROID INPUT (NOT TOO SURE ON THIS)
        */
        float MouseX = static_cast<float>(InputManager::GetInstance().GetMousePosition().first);
        float MouseY = static_cast<float>(InputManager::GetInstance().GetMousePosition().second);

        glm::vec2 mouseUI(MouseX, GraphicsSystem::GetInstance().Getscreen().y - MouseY);

#endif

        /*std::cout << "Screen X: " << GraphicsSystem::GetInstance().Getscreen().x << std::endl;
        std::cout << "Screen Y: " << GraphicsSystem::GetInstance().Getscreen().y << std::endl;
*/

        const glm::vec2 pos = ui.position;
        const glm::vec2 size = ui.size;

        return (mouseUI.x >= pos.x && mouseUI.x <= pos.x + size.x &&
            mouseUI.y >= pos.y && mouseUI.y <= pos.y + size.y);
    }




    void UI_Menu::Trigger_Button_Event(ButtonComponent& button, BUTTON_STATE state) {

        Button_Event event{};

        if (state != HOVER && state != TRIGGERED) {
            return; //early return
        }
        else if (state == HOVER) {
            event.Event_Name = button.event_hover; //every frame for now
            //std::cout << "BUTTON EVENT TRIGGER (Hover):   " << button.event_trigger << std::endl;
        }
        else if (state == TRIGGERED) {
            event.Event_Name = button.event_trigger;
            //std::cout << "BUTTON EVENT TRIGGER (Clicked): " << button.event_trigger << std::endl;
        }
        
        event_dispatcher.trigger<Button_Event>(std::move(event));
        
    }

    void UI_Menu::draw_elements(bool value) {

        /*
            FOR NOW, LETS JUST UNIFORM THEM. IN FUTURE, WE CAN ADD MORE CUSTOMISATION
        */

        for (auto entity : AllElements_in_this_overlay) {

            if (registry.all_of<UIComponent>(entity)) {

                /*
                    SWAPPING OF TEXTURES FOR UI COMPONENT
                */
                auto& ui = registry.get<UIComponent>(entity);
                ui.visible = value;

            }
        }
    }

    void UI_Menu::removeAllEntities_Reg() {

        for (auto entity : AllElements_in_this_overlay) {

            if (!registry.valid(entity)) {
                continue;
            }

            registry.destroy(entity);
          
        }

        AllElements_in_this_overlay.clear();

    }

//#endif
}
