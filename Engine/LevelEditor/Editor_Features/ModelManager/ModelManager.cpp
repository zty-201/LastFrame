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

//Entt reflection system
#include <entt/meta/meta.hpp>
#include <entt/meta/resolve.hpp>
//Entt ECS system
#include <entt/entity/registry.hpp>
#include "System_LevelEditor.hpp"

//Casting meta data to type.
#include "ReflectionRegister.hpp"

#include "ModelManager.hpp"
#include "imgui.h"
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
    };


    ModelManager::ModelManager() {
        LOGI("Lfeature: Model Manager CREATED");
    }
    ModelManager::~ModelManager() {

    }
    void ModelManager::Initialize() {

    }
    void ModelManager::Run() {

        //// Give size once; don't lock it every frame
        ImGui::SetNextWindowSize(ImVec2(420, 260), ImGuiCond_FirstUseEver);

        if (ImGui::Begin("Model Manager", &is_enabled)) {
            
            ImGui::Text("Sample testing of model drag drop");

            for (const auto& asset : assets)
            {               
                // Show an image as a button
                if (ImGui::ImageButton(asset.filename, (ImTextureID)asset.tex, { 100, 100 }, { 0, 1 }, { 1, 0 }))
                {
                    //std::cout<< std::string(asset.filename) + " clicked\n";
                }

                
                if (ImGui::BeginDragDropSource())
                {
                    ImGui::SetDragDropPayload("Models", asset.filename, strlen(asset.filename) + 1);

                    ImGui::TextUnformatted(asset.filename);
                    ImGui::Image((ImTextureID)asset.tex, ImVec2(50, 50),ImVec2(0, 1), ImVec2(1, 0));


                    ImGui::EndDragDropSource();
                }

                ImGui::SameLine(); // show multiple per row
            }

            
            
        }

        ImGui::End();
    }
    void ModelManager::Close() {
        LOGI("model manager closed");
    }

}
#endif
