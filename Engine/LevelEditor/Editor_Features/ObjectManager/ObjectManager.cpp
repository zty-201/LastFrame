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

//For creation of instance where camera is looking.
#include "GraphicsSystem.h"
#include "CameraComponent.h"
#include "TransformComponent.h"
#include "Mesh.h"
#include "MeshComponent.h"

namespace EngineFramework
{
    ObjectManager::ObjectManager() {
        LOGI("Lfeature: ObjectManager CREATED");
    }
    ObjectManager::~ObjectManager() {

    }
    void ObjectManager::Initialize() {

    }

    static inline Mesh::AABB TransformAABB(const Mesh::AABB& box, const glm::mat4& M) {
        const glm::vec3 c = box.center();
        const glm::vec3 e = box.halfExtents();
        const glm::vec3 newC = glm::vec3(M * glm::vec4(c, 1.0f));
        const glm::mat3 A = glm::mat3(M);
        const glm::mat3 AbsA = glm::mat3(glm::abs(A[0]), glm::abs(A[1]), glm::abs(A[2]));
        const glm::vec3 newE = AbsA * e;
        Mesh::AABB out;
        out.minV = newC - newE;
        out.maxV = newC + newE;
        return out;
    }

    void ObjectManager::Run() {

        // Copy entities to a vector to sort
        std::vector<entt::entity> entities;

        // Iterate through the sparse set (all alive entities)
        for (auto entity : registry.view<entt::entity>()) {
            entities.push_back(entity);
        }

        // Sort ascending
        std::sort(entities.begin(), entities.end(), [](entt::entity a, entt::entity b) {
            return (uint32_t)a < (uint32_t)b;
            });

        //// Give size once; don't lock it every frame
        ImGui::SetNextWindowSize(ImVec2(420, 260), ImGuiCond_FirstUseEver);

        if (ImGui::Begin("Object Manager", &is_enabled)) {
            //Buttons at the top
            top_buttons();
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
                    LevelEditor::GetInstance().variables.selected_objectID = selected_entity;
                    LOGI(("Selected Entity: " + std::to_string((uint32_t)selected_entity)).c_str());

                    if (registry.all_of<MeshComponent>(selected_entity)) {
                        const auto& mc = registry.get<MeshComponent>(selected_entity);

                        // Build object-space AABB from MeshComponent authoring values
                        Mesh::AABB obj{};
                        obj.minV = glm::vec3(mc.localMin[0], mc.localMin[1], mc.localMin[2]);
                        obj.maxV = glm::vec3(mc.localMax[0], mc.localMax[1], mc.localMax[2]);

                        // World-space if we also have a transform
                        if (registry.all_of<TransformComponent>(selected_entity)) {
                            const auto& tf = registry.get<TransformComponent>(selected_entity);
                            const Mesh::AABB world = TransformAABB(obj, tf.modelMatrix);

                            LOGI("[AABB] ent=%u  "
                                "objMin(%.3f,%.3f,%.3f) objMax(%.3f,%.3f,%.3f)  "
                                "worldMin(%.3f,%.3f,%.3f) worldMax(%.3f,%.3f,%.3f)",
                                (uint32_t)selected_entity,
                                obj.minV.x, obj.minV.y, obj.minV.z,
                                obj.maxV.x, obj.maxV.y, obj.maxV.z,
                                world.minV.x, world.minV.y, world.minV.z,
                                world.maxV.x, world.maxV.y, world.maxV.z);
                        }
                        else {
                            LOGI("[AABB] ent=%u  objMin(%.3f,%.3f,%.3f) objMax(%.3f,%.3f,%.3f)  (no TransformComponent)",
                                (uint32_t)selected_entity,
                                obj.minV.x, obj.minV.y, obj.minV.z,
                                obj.maxV.x, obj.maxV.y, obj.maxV.z);
                        }
                    }
                    else {
                        LOGI("[AABB] ent=%u  (no MeshComponent)", (uint32_t)selected_entity);
                    }
                }

                // Check for right-click while the item is hovered/active
                if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                    // Open the popup menu attached to this item
                    ImGui::OpenPopup("EntityContextMenu");
                }

                std::string popupID = "EntityContextMenu##" + std::to_string((uint32_t)entity);

                // Check for right-click on the selectable item AND start the menu
                if (ImGui::BeginPopupContextItem(popupID.c_str())) { // <-- Use the unique ID here!

                    // This is safe: sets the context to the currently right-clicked entity
                    selected_entity = entity;

                    // --- 2. UNIQUE MENU ITEM LABEL ---
                    // Optional: Ensure the delete text itself is unique for clarity, though not mandatory
                    std::string deleteLabel = "Delete Entity##" + std::to_string((uint32_t)entity);

                    if (ImGui::MenuItem(deleteLabel.c_str())) {

                        // 1. Destroy the entity
                        registry.destroy(entity);

                        // 2. Clear selection
                        if (LevelEditor::GetInstance().variables.selected_objectID == entity) {
                            LevelEditor::GetInstance().variables.selected_objectID = entt::null;
                        }

                        ImGui::EndPopup();
                        break; // Exit the loop safely
                    }

                    ImGui::EndPopup();
                }

                // Optional: hover highlight (ImGui automatically applies a default hover style)
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Entity ID: %u", (uint32_t)entity);
                }

            }
            
        }
        ImGui::End();
    }
    void ObjectManager::Close() {
        LOGI("object manager closed");
    }

    void ObjectManager::top_buttons()
    {
        if (ImGui::Button("Create Empty Object"))
        {
            entt::entity instance = registry.create();
            registry.emplace<TransformComponent>(instance);
            UNREFERENCED_PARAMETER(instance);
        }
        if (ImGui::Button("Create Empty Object At Camera"))
        {
            entt::entity instance = registry.create();
            TransformComponent instance_transform{};

            //Find the position in front of where the camera is looking.
            entt::entity camera = GraphicsSystem::GetInstance().GetFirstActiveCamera();
            if (camera != entt::null)
            {
                CameraComponent& cam_comp = registry.get<CameraComponent>(camera);
                instance_transform.position = Camera::GetFowardLookPosition(cam_comp, 0.5f);

            }//Otherwise if no active camera found then use default position at 0,0,0.
            else
            {
                LOGD("%s", "No Active Camera Found, creating entity at(0, 0, 0)");
            }
            registry.emplace<TransformComponent>(instance, instance_transform);
        }
    }
}
#endif
