/*****************************************************************/
/*!
\file   PropertyManager.cpp
\author Neo HuiZong
\par    neo.h\@digipen.edu
\date   October 2025
\brief
This file contains the definitions of the functions that are needed to
decide which entity to inspect for the inspector.
Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* *******************************************************************/
#if defined(PLATFORM_WINDOWS) && defined(_DEBUG)

#include "pch.hpp"
#include "PropertyManager.hpp"
#include "InspectorPanel.hpp"
#include "System_GameObjectSystem.hpp"
#include "Level_Editor.hpp"              // ensure LevelEditor is declared
#include "Components/CameraComponent.h"  

using EngineFramework::GameObjectSystem;

namespace EngineFramework {

    // Pick the first entity found in any storage (no loops that trigger C4702)
    static entt::entity pick_any_entity(entt::registry& reg) {
        for (auto&& pair : reg.storage()) {
            auto& storage = pair.second;
            if (storage.size() == 0) continue;

            // Take the first entity from this storage
            entt::entity ent = *storage.begin();
            return ent;                      // First one is enough
        }
        return entt::null;
    }

    void LFeature_PropertyManager::Initialize() {}

    void LFeature_PropertyManager::Run() {
        if (!is_enabled) return;

        auto& reg = GameObjectSystem::GetRegistry();

        /*
            Choosing whether to display entity or model.

            First check if both are invalid or none selected. If so then pick any entity and set instance_selected.
            else if latest_selected indicates which type was selected recently. Prioritise that type if the ID is valid.
            Else the id for that type is Invalid (only the other is valid), so
            - if instance_selected --> use model id and set model_selected.
            - if model_selected --> use instance id and set instance_selected.
        */
        iEditorView& selected_var = LevelEditor::GetInstance().variables;
        bool isEntityValid = registry.valid(selected_var.selected_objectID);
        bool isModelValid = AssetManagerSystem::HasResource(selected_var.selected_modelID);

        //Nothing valid is selected.
        if (selected_var.latest_selected == iEditorView::Latest_Selected::NONE || (!isEntityValid && !isModelValid))
        {
            //Pick a random entity.
            selected_var.selected_objectID = pick_any_entity(reg); //May be null, but let the panel be drawn anyways.
            selected_var.latest_selected = iEditorView::Latest_Selected::INSTANCE_SELECTED;
        }
        //For instance_selected, if entity is not valid but model is then default to model.
        //Same goes for model_selected.
        else if (selected_var.latest_selected == iEditorView::Latest_Selected::INSTANCE_SELECTED)
        {
            //Check if valid or not.
            if (!isEntityValid) selected_var.latest_selected = iEditorView::Latest_Selected::MODEL_SELECTED;
        }
        else //Model Selected
        {
            //Check if valid or not.
            if (!isModelValid) selected_var.latest_selected = iEditorView::Latest_Selected::INSTANCE_SELECTED;
        }
        
        bool is_display_entity_properties = (selected_var.latest_selected == iEditorView::Latest_Selected::INSTANCE_SELECTED);
        // Draw inspector even if ent is still null; the panel handles that message
        EditorFeatures::DrawInspector(reg, selected_var.selected_objectID, selected_var.selected_modelID, is_display_entity_properties);
    }

    void LFeature_PropertyManager::Debug_SpawnTestEntityAndSelect() {
        //auto& reg = GameObjectSystem::GetInstance().GetRegistry();
        auto& reg = GameObjectSystem::GetRegistry();
        // Creaete a test entity and give it a component you know is reflected
        const entt::entity ent = reg.create();

        // If camera component has required constrcutor
        if (!reg.any_of<CameraComponent>(ent)) {
            reg.emplace<CameraComponent>(ent);
        }

        LevelEditor::GetInstance().variables.selected_objectID = ent;
        LevelEditor::GetInstance().variables.latest_selected = iEditorView::Latest_Selected::INSTANCE_SELECTED;
    }

    void LFeature_PropertyManager::Close() {}

} // namespace EngineFramework


//#include "pch.hpp"
//#include "PropertyManager.hpp"
//#include "InspectorPanel.hpp"
//#include "System_GameObjectSystem.hpp"
//
//using EngineFramework::GameObjectSystem;
//
//namespace EngineFramework {
//	void LFeature_PropertyManager::Initialize() {
//
//	}
//
//	// Helper: pick the first entity found in any storage
//	static entt::entity pick_any_entity(entt::registry& reg) {
//		for (auto&& [tid, storage] : reg.storage()) {
//			if (storage.size() == 0) continue;          // or storage.empty() if available
//			// storage is iterable: begin()/end() yield entt::entity
//			for (entt::entity ent : storage) {
//				return ent;                              // first one is enough
//			}
//		}
//		return entt::null;
//	}
//	
//	void LFeature_PropertyManager::Run() {
//
//		//if (!is_enabled) {
//		//	return;
//		//}
//
//		auto& reg = GameObjectSystem::GetInstance().GetRegistry();
//
//		entt::entity ent = LevelEditor::GetInstance().variables.selected_objectID;
//		
//		if (ent == entt::null || !reg.valid(ent)) {
//			if (ent == entt::null || !reg.valid(ent)) {
//				ent = pick_any_entity(reg);                    // no component headers needed
//			}
//		}
//
//
//		//if (ent == entt::null || !reg.valid(ent)) {
//		//	ImGui::Begin("Properties");
//		//	ImGui::TextUnformatted("No entity selected");
//		//	ImGui::End();
//		//	return;
//		//}
//
//		//ImGui::SetNextWindowPos(ImVec2(20, 60), ImGuiCond_FirstUseEver);
//
//		EditorFeatures::DrawInspector(reg, ent);
//	}
//
//	void LFeature_PropertyManager::Close() {}
//
//}
//

#endif