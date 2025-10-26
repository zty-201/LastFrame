/*
*****************************************************************/
/*!
\file   ModelManager.cpp
\author Tan Yee Tong, Joel Lee Jie
\par    yeetong.t\@digipen.edu, joeljie.lee\@digipen.edu
\date   October 2025
\brief
This file shows the models currently loaded in the scene of the level editor
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
#include "entt/core/hashed_string.hpp"

//Models (Archtypes) and Gameobject System
#include "Archtype.hpp"
#include "System_GameObjectSystem.hpp"
#include "ModelManager.hpp"
#include "imgui.h"

//For display of model image
#include "TextureComponent.h"
#include "scene.h"
//#include <iostream>
namespace EngineFramework
{
    /*
        this is a sample, we will replace will the model(aka objects) being loaded
    
    */
    struct Asset {
        ImTextureID tex;
        const char* filename;
    };

    Asset assets[] = {

    { (ImTextureID)0, "Model 1" },
    { (ImTextureID)0, "Model 2" },
    { (ImTextureID)0, "Model 3" },
    { (ImTextureID)0, "Model 4" },
    { (ImTextureID)0, "Model 5" },
    { (ImTextureID)0, "Model 6" },
    { (ImTextureID)0, "Model 7" },
    { (ImTextureID)0, "Model 8" },
    { (ImTextureID)0, "Model 9" },
    { (ImTextureID)0, "Model 10" },
    { (ImTextureID)0, "Model 11" },
    { (ImTextureID)0, "Model 12" },

    };


    ModelManager::ModelManager() {
        LOGI("Lfeature: Model Manager CREATED");
    }
    ModelManager::~ModelManager() {

    }
    void ModelManager::Initialize() {

    }
    void ModelManager::Run() {

        std::vector<std::pair<xresource::instance_guid, std::shared_ptr<Archtype>>> archtype_arr = AssetManagerSystem::GetAllResourceOfType<Archtype>(AssetManagerSystem::ARCHTYPE_GUID);
        //// Give size once; don't lock it every frame
        ImGui::SetNextWindowSize(ImVec2(420, 260), ImGuiCond_FirstUseEver);

        if (ImGui::Begin("Model Manager", &is_enabled)) {

            //==Table formatting
            static int per_row = 3;
            ImGui::InputInt("Show models per row: ", &per_row, 1, 1);

            static int spacing = 20;
            ImGui::InputInt("Spacing: ", &spacing, 2, 10);

            //lock at min 3 and max 15
            per_row = std::clamp(per_row, 3, 15);
            spacing = std::clamp(spacing, 10, 50);

            /*float*/

            const float avail = ImGui::GetContentRegionAvail().x;
            const float spacingF = (float)spacing; 
            const float cell_w = floorf((avail - spacingF* (per_row +1)) / per_row);

            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(spacingF, 8.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(spacingF*0.5f, 6.0f)); // nice outer/inner padding
            
            /*
                Display all archtypes in table format. 
                In later improvements, sort the archtypes by string_label and display them in their respective tabs.
                - This label should be part of the archtype instead of being another component, to prevent copying to instances.
                
                Display
                - Name
                - Texture image if possible

                Also enable dragdrop into scene (create instance with default transform at where camera is pointing, with option to choose normalized or world pos).
                Allow user to copy and make new archtypes (of different name).
                Allow user to make new archtype.
            */
            entt::id_type texture_comp_id = entt::resolve<TextureComponent>().id();
            if (ImGui::BeginTable("model-grid", per_row, ImGuiTableFlags_SizingStretchSame| ImGuiTableFlags_PadOuterX| ImGuiTableFlags_NoBordersInBody))
            {
                for (int i = 0; i < archtype_arr.size(); i++)
                {
                    //==Setting of table element
                    if (i % per_row == 0) ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(i % per_row);
                    ImGui::PushID(i);

                    ImGui::BeginGroup();


                    //==Accessing archtype data and name
                    auto pair = archtype_arr[i];
                    xresource::instance_guid archtype_guid = pair.first;
                    std::shared_ptr<Archtype> archtype = pair.second;

                    std::string archtype_name = AssetManagerSystem::GetName(archtype_guid);
                    std::vector<entt::meta_any> component_arr = archtype->component_arr;
                    GLuint texture_id{};
                    /*
                        Iterate over all component types and get the texture id if possible.
                    */
                    for (const entt::meta_any& comp : component_arr)
                    {
                        if (comp.type().id() != texture_comp_id) continue;

                        //Get the texture asset for drawing
                        xresource::full_guid texture_guid = comp.cast<TextureComponent>().texture_guid;

                        try
                        {
                            std::shared_ptr<Texture> texture = AssetManagerSystem::GetResource<Texture()
                        }
                        catch (...)
                        {

                        }
                        break;
                    }


                    //==Drawing of archtype element.
                    if (ImGui::ImageButton(, (ImTextureID)0, ImVec2(cell_w, cell_w), { 0, 1 }, { 1, 0 }))
                    {

                    }

                    //==Cleanup of table element
                    ImGui::EndGroup();
                    ImGui::PopID();
                }
               /* for (auto asset : assets)
                {
                    if (i % per_row == 0) ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(i % per_row);
                    ImGui::PushID(i);

                    ImGui::BeginGroup();
                    if (ImGui::ImageButton(asset.filename, (ImTextureID)0, ImVec2(cell_w, cell_w), { 0, 1 }, { 1, 0 }))
                    {

                    }

                    if (ImGui::BeginDragDropSource())
                    {
                        ImGui::SetDragDropPayload("Models", asset.filename, strlen(asset.filename) + 1);

                        ImGui::TextUnformatted(asset.filename);
                        ImGui::Image((ImTextureID)0, ImVec2(50, 50), ImVec2(0, 1), ImVec2(1, 0));

                        ImGui::EndDragDropSource();
                    }

                    ImGui::Text(asset.filename);

                    ImGui::EndGroup();

                    ImGui::PopID();

                    i++;*/

                //}

                ImGui::EndTable();
            }

            ImGui::PopStyleVar(2);

            /*
            * O
            
                //Get the archtypes stored in asset manager.
            std::vector<std::pair<xresource::instance_guid, std::shared_ptr<Archtype>>> archtype_arr = AssetManagerSystem::GetAllResourceOfType<Archtype>(AssetManagerSystem::ARCHTYPE_GUID);

            int i = 0;

            for (const auto& archtype_pair : archtype_arr)
            {
                std::string archtype_name = AssetManagerSystem::GetName(archtype_pair.first);
                if (archtype_name.empty()) continue; //Cannot serialize an archtype with no name or value.

                ImGui::BeginGroup();

                if (ImGui::ImageButton(archtype_name.c_str(), (ImTextureID)0, ImVec2(image_width, image_width), { 0, 1 }, { 1, 0 }))
                {

                }

                if (ImGui::BeginDragDropSource())
                {
                    ImGui::SetDragDropPayload("Models", archtype_name.c_str(), strlen(archtype_name.c_str()) + 1);

                    ImGui::TextUnformatted(archtype_name.c_str());
                    ImGui::Image((ImTextureID)0, ImVec2(50, 50), ImVec2(0, 1), ImVec2(1, 0));

                    ImGui::EndDragDropSource();
                }

                ImGui::Text(archtype_name.c_str());

                ImGui::EndGroup();

                const bool lastInRow = ((i + 1) % per_row) == 0;
                if (!lastInRow) ImGui::SameLine(0.0f, (float)spacing);

                i++;

            }

            ImGui::Text("* Sample testing of model drag drop *");
            ImGui::PopStyleVar();
            
            
            
            */

            

        }
        ImGui::End();
        
    }
    void ModelManager::Close() {
        LOGI("model manager closed");
    }

}
#endif
