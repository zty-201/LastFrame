/*****************************************************************/
/*!
\file   InspectorPanel.cpp
\author Neo HuiZong
\par    neo.h\@digipen.edu
\date   October 2025
\brief
This file contains the definitions of the functions that are needed to 
set up the window and fetches the components meta references.
Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* *******************************************************************/

#include "pch.hpp"
#include "InspectorPanel.hpp"
#include "MetaInspector/MetaInspector.hpp"
#include <imgui.h>

namespace EditorFeatures {

    void DrawInspector(entt::registry& reg, entt::entity e) {
        ImGui::Begin("Inspector");

        const entt::id_type GET_REF_ID = entt::hashed_string::value("get_ref");

        // Diagnostics at the top
        //ImGui::Text("Entity: 0x%08X  valid:%d  selected:%s",
        //    (unsigned)entt::to_integral(e),
        //    reg.valid(e) ? 1 : 0,
        //    e == entt::null ? "null" : "yes");

        //int stor = 0, owned = 0, meta = 0, withGetRef = 0;
        //for (auto&& [tid, storage] : reg.storage()) {
        //    ++stor;
        //    if (!storage.contains(e)) continue; ++owned;
        //    auto mt = entt::resolve(storage.info());
        //    if (!mt) continue; ++meta;
        //    if (mt.func(GET_REF_ID)) ++withGetRef;   // STRING lookup (safer)
        //}
        //ImGui::Text("storages:%d  owned:%d  meta:%d  get_ref:%d", stor, owned, meta, withGetRef);
        //ImGui::Separator();

        if (e == entt::null || !reg.valid(e)) {
            ImGui::TextDisabled("No entity selected.");
            ImGui::End();
            return;
        }

        // Actual draw
        for (auto&& [type_id, storage] : reg.storage()) {
            if (!storage.contains(e)) continue;

            entt::meta_type mt = entt::resolve(storage.info());
            if (!mt) continue;

            auto get_ref = mt.func(GET_REF_ID);      // STRING lookup
            if (!get_ref) continue;

            entt::meta_any comp = get_ref.invoke({}, e);
            if (!comp) continue;

            const char* header = mt.name();
            if (!header || !*header) header = "Component";

            if (ImGui::CollapsingHeader(header, ImGuiTreeNodeFlags_DefaultOpen)) {
                EditorFeatures::DrawMetaObject(mt, comp);
            }
        }

        ImGui::End();
    }

	//void DrawInspector(entt::registry& reg, entt::entity entity) {
	//	ImGui::Begin("Properties");
	//	if (entity == entt::null) {
	//		ImGui::TextUnformatted("Nothing selected.");
	//		ImGui::End();
	//		return;
	//	}

	//	const auto GET_REF_ID = entt::hashed_string::value("get_ref");

	//	// Iterate through component storage and draw those that the entity actually has
	//	for (auto&& [type_id, storage] : reg.storage()) {
	//		if (!storage.contains(entity)) {
	//			continue;
	//		}
	//		entt::meta_type mt = entt::resolve(storage.info());

	//		if (!mt) {
	//			continue;
	//		}

	//		auto get_ref = mt.func(GET_REF_ID);
	//		if (!get_ref) {
	//			continue; // Dont expose to inspector
	//		}

	//		entt::meta_any component = get_ref.invoke({}, entity); // Live reference of the component

	//		// Foldout header = type name registered in the reflectionregister side
	//		const char* header = mt.name();
	//		if (!header || !*header) {
	//			header = "Component";
	//		}

	//		if (ImGui::CollapsingHeader(header, ImGuiTreeNodeFlags_DefaultOpen)) {
	//			EditorFeatures::DrawMetaObject(component); // Call for metainspector function
	//		}
	//	}

	//	ImGui::End();
	//}
}
