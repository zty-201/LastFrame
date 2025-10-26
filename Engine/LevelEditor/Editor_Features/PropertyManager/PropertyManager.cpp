#include "pch.hpp"
#include "PropertyManager.hpp"
#include "InspectorPanel.hpp"
#include "System_GameObjectSystem.hpp"
#include "Level_Editor.hpp"              // ensure LevelEditor is declared
#include "Components/CameraComponent.h"   // <-- adjust path/name to yours


using EngineFramework::GameObjectSystem;

namespace EngineFramework {

    // Pick the first entity found in any storage (no loops that trigger C4702)
    static entt::entity pick_any_entity(entt::registry& reg) {
        for (auto&& pair : reg.storage()) {
            auto& storage = pair.second;
            if (storage.size() == 0) continue;

            // take the first entity from this storage
            entt::entity ent = *storage.begin();
            return ent;                      // first one is enough
        }
        return entt::null;
    }

    void LFeature_PropertyManager::Initialize() {}

    void LFeature_PropertyManager::Run() {
        if (!is_enabled) return;   // match your feature pattern (like Profiler)

        auto& reg = GameObjectSystem::GetRegistry();

        // current selection
        entt::entity ent = LevelEditor::GetInstance().variables.selected_objectID;

        // fallback (no duplicate condition)
        if (ent == entt::null || !reg.valid(ent)) {
            ent = pick_any_entity(reg);
        }

        // draw inspector even if ent is still null; the panel handles that message
        EditorFeatures::DrawInspector(reg, ent);
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
