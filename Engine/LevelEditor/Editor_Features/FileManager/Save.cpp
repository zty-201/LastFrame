/*
*****************************************************************/
/*!
\file   Save.cpp
\author Tan Yee Tong
\par    yeetong.t\@digipen.edu
\date   September 2025
\brief
This file contains functions that saves the scene file that is currently being loaded
Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* *******************************************************************/


#if defined(PLATFORM_WINDOWS) && defined(_DEBUG)
#include "pch.hpp"

//File management, JSON file parser/writer
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

//Entt reflection system
#include <entt/meta/meta.hpp>
#include <entt/meta/resolve.hpp>
//Entt ECS system
#include <entt/entity/registry.hpp>
#include "System_LevelEditor.hpp"

//Casting meta data to type.
#include "ReflectionRegister.hpp"
#include "AssetManager.hpp"
#include "Save.hpp"
//#include <iostream>
#include "System_GameObjectSystem.hpp"
#include "InputManager.hpp"
#include "AssetWindow.hpp"
#include "UISystem.hpp"
namespace EngineFramework
{
    Save::Save() {

        this_ptr = this;
        LOGI("Lfeature: Save CREATED");
    }
    Save::~Save() {

    }
    void Save::Initialize() {

    }
    void Save::Run() {

        static char inputBuffer[128] = "";
        const char* POPUP_NAME = "FILE Save As";

        static bool to_open = false;

        //only open once until it closes
        if (is_enabled && !to_open) {
            ImGui::OpenPopup(POPUP_NAME);
            to_open = true;
        }

        if (ImGui::BeginPopupModal(POPUP_NAME, NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
           
            ImGui::InputTextWithHint("##InputField", "Save file as...", inputBuffer, IM_ARRAYSIZE(inputBuffer));
            
            std::string str_buffer = std::string(inputBuffer);
            ImGui::Text("(*please exclude all spaces and include the extension*)");

            if (str_buffer.empty()) {

                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
                ImGui::Text("*Please provide a file name");
                ImGui::PopStyleColor();
            }

            ImGui::Spacing();

            if (ImGui::Button("Cancel"))
            {
                ImGui::CloseCurrentPopup();
                is_enabled = false;
                to_open = false;
            }
            ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - ImGui::GetStyle().FramePadding.x - 60);

            if (ImGui::Button("Save"))
            {
                //only save if the filename is not empty
                if (!str_buffer.empty()) {

                    printf("User entered: %s\n", inputBuffer);
                    ImGui::CloseCurrentPopup();

                    if (!SAVEFILE(str_buffer)) {
                        LOGI("Error saving file. File not saved\n");
                    }
                    is_enabled = false;
                    to_open = false;
                }
                
            }

            ImGui::EndPopup();
        }

        
    }
    void Save::Close() {

    }

    bool Save::SAVEFILE(std::string filename){

        if (filename.empty()) {
            return false;
        }

        if (!save_Archtypes()) return false;
        if (!save_Instances(filename)) return false;
        

        InputManager::GetInstance().key_mapping_codegen();
        InputManager::GetInstance().seralise();
        InputManager::GetInstance().key_mapping_codegen_script();
        AssetWindow::GetInstance().seralise();
        

        return true;

    }

    bool Save::save_Archtypes() {

        
        auto pairs = GameObjectSystem::GetInstance().SerializeAllEntityArchtypes();
        std::string base = AssetManagerSystem::GetAssetFolderPrefix().string() + "Extern/Archtypes/";

        for (auto& [name, json] : pairs)
        {
            std::string path = base + name + ".json";
            std::ofstream Out_File(path, std::ios::binary | std::ios::trunc);

            if (!Out_File.is_open()) return false;

            std::stringstream ss;

            /*
                Serialise here
            */
            ss << json;

            Out_File << ss.str();   // Output the contents of the stringstream
            Out_File.close();
        }

        return true;

    }

    bool Save::save_Instances(std::string filename) {

        std::unordered_set <entt::entity> except_list = UI_System::GetInstance().Seralise_AllMenus();
        std::string path = AssetManagerSystem::GetAssetFolderPrefix().string() + "Extern/Scenes/" + filename;
        std::ofstream Out_File(path);

        if (!Out_File.is_open()) return false;

        std::stringstream ss;

        /*
            Seralise here

        */

        std::string json = EngineFramework::GameObjectSystem::GetInstance().SerializeAllEntityInstances(except_list);

        ss << json;

        Out_File << ss.str(); // Output the contents of the stringstream
        Out_File.close();

        return true;

    }

}
#endif
