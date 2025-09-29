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

#include "ObjectManager.hpp"


namespace EngineFramework
{
    ObjectManager::ObjectManager() {
        LOGI("Lfeature: ObjectManager CREATED");
    }
    ObjectManager::~ObjectManager() {

    }
    void ObjectManager::Initialize() {

    }
    void ObjectManager::Run() {

        // Copy entities to a vector to sort
        std::vector<entt::entity> entities;

        // Iterate through the sparse set (all alive entities)
        for (auto entity : registry.storage<entt::entity>()) {
            entities.push_back(entity);
        }

        // Sort ascending
        std::sort(entities.begin(), entities.end(), [](entt::entity a, entt::entity b) {
            return (uint32_t)a < (uint32_t)b;
            });

        //// Give size once; don't lock it every frame
        ImGui::SetNextWindowSize(ImVec2(420, 260), ImGuiCond_FirstUseEver);

        if (ImGui::Begin("Object Manager", &is_enabled)) {
            ImGui::Text("Scene Hierarchy");

            static entt::entity selected_entity = entt::null;

            for (auto entity : entities) {
                std::string label = "Entity " + std::to_string((uint32_t)entity);
                //ImGui::Text("%s", label.c_str());

                // Determine if this entity is selected
                bool is_selected = (entity == selected_entity);

                // Make the entity a selectable item
                if (ImGui::Selectable(label.c_str(), is_selected)) {
                    selected_entity = entity;
                    LevelEditor::GetInstance().variables.selected_modelID = (uint32_t)selected_entity;
                    LOGI(("Selected Entity: " + std::to_string((uint32_t)selected_entity)).c_str());
                }

                // Optional: hover highlight (ImGui automatically applies a default hover style)
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Entity ID: %u", (uint32_t)entity);
                }

            }
            ImGui::End();
        }
    }
    void ObjectManager::Close() {
        LOGI("object manager closed");
    }

}
#endif
