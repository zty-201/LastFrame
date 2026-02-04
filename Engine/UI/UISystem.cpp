/*
*****************************************************************/
/*!
\file   UISystem.cpp
\author Tan Yee Tong
\par    yeetong.t\@digipen.edu
\date   Jan 2026
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
#include "System_GameObjectSystem.hpp"

#endif

#include "AssetManager.hpp"

namespace EngineFramework
{

//#if defined(DEBUG) && defined(PLATFORM_WINDOWS)
    std::unordered_map<std::string, std::pair<std::unique_ptr<UI_Menu>, bool>> UI_System::Menu_Mapping;

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

        DeserializeAllOverlays();

        //std::cout << "=========================================\n";
        //std::cout << "        DeserializeAllOverlays())        \n";
        //std::cout << "=========================================\n";

        /////////////////// TESTING START ////////////////////
        //menu_test = std::make_unique<UI_Menu>();
        // menu_test->Deseralise("Menu_1");
        //menu_test->Initialize();
        /////////////////// TESTING END ////////////////////

        for (auto& [name, menu] : Menu_Mapping) {

            if (menu.first) {
                menu.first->Initialize();
                menu.second = false;
            }
        }

        current_menu = ""; //none, i havent set default yet

        //std::cout << "=========================================\n";
        //std::cout << "   DONE     UI_System::Initialize()      \n";
        //std::cout << "=========================================\n";
      
    }

    void UI_System::Clean_All() {

        for (auto& [name, menu] : Menu_Mapping) {

            if (menu.first) {
                
                menu.first->removeAllEntities_Reg(); //only update those active ones
                
            }
        }

        Menu_Mapping.clear();
        current_menu.clear();
    }

    void UI_System::DeserializeAllOverlays(){

        Menu_Mapping.clear();
        const std::filesystem::path dir = AssetManagerSystem::GetInstance().GetAssetFolderPrefix().string() + "Extern/Overlays/";
           
        for (const auto& entry : std::filesystem::directory_iterator(dir)) {

            if (!entry.is_regular_file()) continue;

            const std::filesystem::path& path = entry.path();

            if (path.extension() != ".json") continue;
            const std::string menuName = path.stem().string();
            
            DeserializeOverlay(menuName);

        }

        //std::cout << "SIZE of Menu_Mapping: " << Menu_Mapping.size() << std::endl;

    }
    bool UI_System::DeserializeOverlay(std::string menuName){

        auto menu = std::make_unique<UI_Menu>(menuName);
        //std::cout << "DESERALISING OVERLAY: MENU NAME: " << menuName << std::endl;

        if (!menu->Deseralise(menuName)) {
            //std::cout << "!!!!!!!!!ERROR DESERALISING OVERLAY: MENU NAME: "<< menuName << std::endl;
            return false;
        }

        Menu_Mapping.emplace(menuName, std::pair(std::move(menu),false));
        return true;

    }

    std::unordered_set<entt::entity> UI_System::Seralise_AllMenus() {

        std::unordered_set<entt::entity> combined;


        for (auto& [name, menu] : Menu_Mapping) {

            if (menu.first) {
                std::unordered_set<entt::entity> entities = menu.first->Seralise();
                combined.insert(entities.begin(), entities.end());
                menu.second = false; //set to false, this var can only be called via the program not valjue based ah
            }
        }

        return combined;
        
    }

    void UI_System::Update(float deltaTime)
    {
        (void)deltaTime;

        for (auto& [name, menu] : Menu_Mapping) {

            if (menu.first) {  
                if (menu.second) {
                    menu.first->Update(); //only update those active ones
                }                
            }
        }

    }

    void UI_System::PostUpdate(float deltaTime)
    {
        (void)deltaTime;

        
    }
    void UI_System::add_entity_fromMenu(std::string menu, entt::entity e){

        auto it = Menu_Mapping.find(menu);

        if (it != Menu_Mapping.end() && it->second.first) {

            it->second.first->Add_Element(e);

        }

    }
    void UI_System::remove_entity_fromMenu(std::string menu, entt::entity e){

        auto it = Menu_Mapping.find(menu);

        if (it != Menu_Mapping.end() && it->second.first) {

            it->second.first->Remove_Element(e);

        }

    }


    void UI_System::create_overlay(std::string menuName) {

        std::filesystem::path path = AssetManagerSystem::GetInstance().GetAssetFolderPrefix().string() + "Extern/Overlays/" + menuName +".json";

        // Only create if it doesn't exist
        if (!std::filesystem::exists(path))
        {
            std::ofstream out(path);
            if (out.is_open())
            {
                out << "{}";
                out.close();
            }
        }

        auto menu = std::make_unique<UI_Menu>(menuName);
        menu->Initialize();
        Menu_Mapping.emplace(menuName, std::pair(std::move(menu), false));
        //the list should be zero because, there are no entities yet in this list :<
        



    }

    std::vector<entt::entity> UI_System::getMenu_Entities(std::string menuname) {

        //std::cout << "getMenu_Entities from : " << menuname << std::endl;
        auto it = Menu_Mapping.find(menuname);

        if (it != Menu_Mapping.end() && it->second.first) {

            return it->second.first->get_Elements();
           
        }

        return {};

    }

    std::unordered_set<entt::entity> UI_System::getAllMenu_Entities() {

        std::unordered_set<entt::entity> set;

        for (auto& [name, menu] : Menu_Mapping)
        {
            if (!menu.first) continue;

            const auto& elems = menu.first->get_Elements();

            set.insert(elems.begin(), elems.end());
        }

        return set;

    }

    void UI_System::displayMenu(std::string menuname) {

        //std::cout << "hello i am diplaying: " << menuname << std::endl;


        auto it = Menu_Mapping.find(menuname);

        if (it != Menu_Mapping.end() && it->second.first) {
            it->second.first->draw_elements();
            current_menu = menuname;
            it->second.second = true;
        }

        

    }
    void UI_System::closeMenu(std::string menuname) {

        auto it = Menu_Mapping.find(menuname);

        if (it != Menu_Mapping.end() && it->second.first) {
            it->second.first->draw_elements(false);
            current_menu = "";
            it->second.second = false;
        }
    }

    void UI_System::closeMenu() {

        auto it = Menu_Mapping.find(current_menu);

        if (it != Menu_Mapping.end() && it->second.first) {
            it->second.first->draw_elements(false);
            current_menu = "";
            it->second.second = false;
        }

    }


    void UI_System::closeAllMenu() {

        for (auto& [name, menu] : Menu_Mapping) {

            if (menu.first) {
                menu.first->draw_elements(false);
                menu.second = false;
            }
        }

    }

//#endif    

}
