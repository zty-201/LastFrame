/*
*****************************************************************/
/*!
\file   ObjectManager.cpp
\author Tang Yeun, Dean
\par    y.tang\@digipen.edu
\date   September 2025
\brief
This file runs the Scene Hierarchy of the level editor
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
#include <cctype>
//Entt reflection system
#include <entt/meta/meta.hpp>
#include <entt/meta/resolve.hpp>
//Entt ECS system
#include <entt/entity/registry.hpp>
#include "System_LevelEditor.hpp"

//Casting meta data to type.
#include "ReflectionRegister.hpp"

#include "GameplayKeyManager.hpp"
#include "InputManager.hpp"
#include "KeyMapping.hpp"

namespace EngineFramework
{
    static int ToUppercaseCallback(ImGuiInputTextCallbackData* data)
    {
        for (int i = 0; i < data->BufTextLen; ++i)
        {
            data->Buf[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(data->Buf[i])));
        }
        return 0;
    }



    Gameplay_KeyManager::Gameplay_KeyManager() {
        LOGI("Lfeature: Gameplay_KeyManager CREATED");
    }
    Gameplay_KeyManager::~Gameplay_KeyManager() {

    }
    void Gameplay_KeyManager::Initialize() {

    }
    void Gameplay_KeyManager::Run() {

        static std::string action_name = "";
        static const char* POPUP_NAME = "Choose A Key";
        static bool open_popup = false;
        
        // Give size once; don't lock it every frame
        ImGui::SetNextWindowSize(ImVec2(420, 260), ImGuiCond_FirstUseEver);

        if (ImGui::Begin("Input Key Mapping", &is_enabled)) {

            ImGui::SeparatorText("Gameplay Keys");
            ImGui::Dummy(ImVec2(0.0f, 20.f));

            //std::map <std::string, int> key_mapping_data; //json file data


            for (auto it = InputManager::GetInstance().key_mapping_data.begin(); it != InputManager::GetInstance().key_mapping_data.end(); it++)
            {
                const std::string& action = it->first;                
                std::string key_name = ImGui::GetKeyName(Get_IMGUI_Key(it->second));

                ImGui::Text(action.c_str());
                ImGui::SameLine();
                ImGui::SetCursorPosX(350);

                if (ImGui::Button(key_name.c_str(), ImVec2(200, 0)))
                {
                    action_name = action;
                    open_popup = true;
                }

                ImGui::SameLine();

                // Red "-" button
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.90f, 0.20f, 0.20f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.95f, 0.35f, 0.35f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.80f, 0.15f, 0.15f, 1.0f));

                if (ImGui::Button(("-##" + action).c_str()))
                {
                    remove_key(action);
                    ImGui::PopStyleColor(3);
                    break;
                }

                ImGui::PopStyleColor(3);
                
            }





            ImGui::Dummy(ImVec2(0.0f, 20.f));
            ImGui::SeparatorText("Add New Gameplay Keys");
            ImGui::Dummy(ImVec2(0.0f, 20.f));
      
            static char buffer[64] = "";            
            ImGuiInputTextFlags flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackAlways;

            ImGui::InputTextWithHint("##AddAction", "Type to add action", buffer, IM_ARRAYSIZE(buffer), flags, ToUppercaseCallback);           
            ImGui::SameLine();

            if (ImGui::Button("Add")) {
           
                action_name = buffer; //assign action_name to the buffer
                strcpy_s(buffer, ""); //empty

                if (valid_string(action_name)) {
                    open_popup = true;
                }
                 
            }

            ImGui::Dummy(ImVec2(0.0f, 20.f));
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 0, 0, 1));
            ImGui::Text("NOTE: Only letters and '_' are accepted. ");
            ImGui::PopStyleColor(); // back to normal
            
        }
        ImGui::End();

        if (open_popup) {
            ImGui::OpenPopup(POPUP_NAME); //if edited, open popup
            open_popup = false;            
        }

        ImGui::SetNextWindowSize(ImVec2(450, 300), ImGuiCond_Once);
        if (ImGui::BeginPopupModal(POPUP_NAME, NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove))
        {
            //float region_right = ImGui::GetWindowContentRegionMax().x;
            ImGui::TextWrapped("Press any key on your keyboard to assign the new action with a key");           
            ImGui::Dummy(ImVec2(0.0f, 20.f));
            ImGui::Text(std::string("ACTION: " + action_name).c_str());

            static bool is_key_entered = false;
            static ImGuiKey key_entered = ImGuiKey_None;

            if (!is_key_entered) {

                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 0, 0, 1));
                ImGui::Text("Imgui is listening ... ");
                ImGui::PopStyleColor(); // back to normal
                for (int key = ImGuiKey_NamedKey_BEGIN; key < ImGuiKey_NamedKey_END; key++)
                {
                    ImGuiKey k = static_cast<ImGuiKey>(key);

                    // Skip mouse keys directly
                    if (k >= ImGuiKey_MouseLeft && k <= ImGuiKey_MouseWheelY)
                        continue;

                    // Skip mouse buttons/wheel, but still detect keyboard
                    if (ImGui::IsKeyPressed(k))
                    {
                        key_entered = k;
                        is_key_entered = true;
                        break;
                    }
                }
            }
            else {

                std::string text = "Key entered: " + std::string(ImGui::GetKeyName(key_entered));
                ImGui::Text(text.c_str());
                ImGui::SameLine();

                if (ImGui::Button("Change"))
                {
                    is_key_entered = false;
                }
            }

            
            float button_height = 30.f;
            float button_y = ImGui::GetWindowContentRegionMax().y - button_height - ImGui::GetStyle().FramePadding.y;
            ImGui::SetCursorPosY(button_y);

            if (ImGui::Button("Cancel"))
            {
                ImGui::CloseCurrentPopup();
                is_key_entered = false;

            }

            if (is_key_entered) {

                ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - ImGui::GetStyle().FramePadding.x - 102);

                if (ImGui::Button("Confirm"))
                {
                    ImGui::CloseCurrentPopup();
                    add_key(action_name, key_entered);
                    is_key_entered = false;
                }
            }

            ImGui::EndPopup();
        }

    }
    void Gameplay_KeyManager::Close() {
        LOGI("Gameplay_KeyManager closed");
    }

    void Gameplay_KeyManager::add_key(std::string action, ImGuiKey imgui_key) {

        InputManager::GetInstance().key_mapping_data[action] = Get_GLFW_Key(imgui_key);
        
    }
    void Gameplay_KeyManager::remove_key(std::string action) {

        auto it = InputManager::GetInstance().key_mapping_data.find(action);
        if (it != InputManager::GetInstance().key_mapping_data.end()) {
            InputManager::GetInstance().key_mapping_data.erase(it);
        }
         
    }

    bool Gameplay_KeyManager::valid_string(std::string str){

        if (str.empty())
            return false;
            
        if (!std::isalpha(static_cast<unsigned char>(str.back())) && str.back() != '_')
            return false;
       
        for (char c : str)
        {
            if (!std::isalpha(static_cast<unsigned char>(c)) && c != '_')
                return false;
        }

        return true;
    }

}
#endif
