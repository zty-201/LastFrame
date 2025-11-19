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
#if defined(PLATFORM_WINDOWS) && defined(_DEBUG)

#include "pch.hpp"
#include "InspectorPanel.hpp"
#include "MetaInspector/MetaInspector.hpp"
#include "System_GameObjectSystem.hpp"
#include "Assets/System/AssetManager.hpp"
#include "AssetWindow/AssetWindow.hpp"
#include <sstream>
#include <imgui.h>
#include <filesystem>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <vector>
#include <utility>
#include <cstdint>
//For scopeguard CALL_AT_END.
#include "Level_Editor.hpp"
namespace EditorFeatures {

	namespace {

		using namespace entt::literals;

		struct ComponentDropMatch {
			entt::meta_type type;
			entt::meta_data field;
			std::string component_name;
			std::string field_name;
		};

		struct ComponentDropContext {
			std::unordered_map<std::string, std::vector<ComponentDropMatch>> by_extension;
			std::vector<std::string> payload_types;
		};

		static std::string to_lower_copy(std::string value) {
			std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
				return static_cast<char>(std::tolower(c));
				});
			return value;
		}

		// To check the entt meta registry down to actual components by requiring the emplace function so that user can only add components that can be instantiated
		static bool is_component_meta_type(const entt::meta_type& meta) {
			if (!meta || !meta.is_class()) {
				return false;
			}
			return static_cast<bool>(meta.func("emplace"_hs));
		}

		// Check which component fields accept which file extensions
		// Build a map from normalized extensions to component/field pairs that should react accordingly when a file type is dropped
		// When a file is dropped onto "Add Component" buttom, look up the extension inside a map to decide whether to add a component, update a field or show feedback message
		static ComponentDropContext build_drop_context(EngineFramework::AssetWindow& asset_window) {
			ComponentDropContext ctx{};

			std::unordered_set<std::string> seen_payloads{};
			auto register_payload = [&](const std::string& payload) {
				if (seen_payloads.insert(payload).second) {
					ctx.payload_types.push_back(payload);
				}
				};

			// Accept known payload types from the asset browser by default.
			register_payload("GENERAL");
			register_payload("Textures");
			register_payload("Audio");
			register_payload("Scripts");

			for (const auto& [type_id, meta] : entt::resolve()) {
				if (!meta || !meta.is_class()) {
					continue;
				}

				const char* component_name = meta.name();
				if (!component_name || component_name[0] == '\0') {
					continue;
				}
				const std::string component_key{ component_name };

				for (const auto& [data_id, variable] : meta.data()) {
					const char* field_name = variable.name();
					if (!field_name || field_name[0] == '\0') {
						continue;
					}

					std::vector<std::string> tags{};
					if (!asset_window.get_tags(component_key, field_name, tags)) {
						continue;
					}

					for (std::string tag : tags) {
						if (tag.empty()) {
							continue;
						}

						tag = to_lower_copy(tag);
						ctx.by_extension[tag].push_back(ComponentDropMatch{ meta, variable, component_key, std::string{ field_name } });

						register_payload(tag);
					}
				}
			}

			return ctx;
		}

		// Based on the path, assign the correct field for the component when dropping in
		// Take the component instance, the meta data for the specific field and the dropped asset path and set the field accordingly
		static bool assign_field_from_path(entt::meta_any& component, const entt::meta_data& field, const std::filesystem::path& asset_path) {
			if (!component || !field) {
				return false;
			}

			const entt::meta_type field_type = field.type();
			if (!field_type) {
				return false;
			}

			const std::string normalized = asset_path.generic_string();

			if (field_type == entt::resolve<std::string>()) {
				return field.set(component, normalized);
			}

			if (field_type == entt::resolve<std::filesystem::path>()) {
				std::filesystem::path value = asset_path;
				try {
					std::filesystem::path root = EngineFramework::AssetManagerSystem::GetAssetFolderPrefix();
					if (root.empty()) {
						root = std::filesystem::path{};
					}

					std::error_code ec_root{};
					std::error_code ec_path{};
					std::filesystem::path abs_root = std::filesystem::absolute(root, ec_root);
					std::filesystem::path abs_path = std::filesystem::absolute(asset_path, ec_path);
					if (!ec_root && !ec_path) {
						std::error_code ec_rel{};
						std::filesystem::path relative = std::filesystem::relative(abs_path, abs_root, ec_rel);
						if (!ec_rel && relative.has_filename() && relative.string().find("..") == std::string::npos) {
							value = relative.lexically_normal();
						}
					}
				}
				catch (...) {
					// Fall back to original asset_path if any conversion fails.
				}
				return field.set(component, value);
			}

			if (field_type == entt::resolve<xresource::full_guid>()) {
				using EngineFramework::AssetManagerSystem;

				const std::string asset_name = asset_path.stem().string();
				if (asset_name.empty()) {
					return false;
				}

				const auto to_guid = [&](const xresource::type_guid& type_guid) -> xresource::full_guid {
					return AssetManagerSystem::GetFullGuid(type_guid, asset_name);
					};

				const auto tag = static_cast<const AssetPickerTag*>(field.custom());
				xresource::full_guid resolved{};

				auto try_texture = [&]() {
					resolved = to_guid(AssetManagerSystem::TEXTURE_GUID);
					if (resolved.empty()) {
						resolved = to_guid(AssetManagerSystem::CUBEMAP_GUID);
					}
					};

				auto try_audio = [&]() {
					resolved = to_guid(AssetManagerSystem::AUDIO_GUID);
					};

				auto try_script = [&]() {
					resolved = to_guid(AssetManagerSystem::SCRIPT_GUID);
					};

				if (tag) {
					switch (tag->kind) {
					case AssetPickerKind::Texture:
						try_texture();
						break;
					case AssetPickerKind::Audio:
						try_audio();
						break;
					case AssetPickerKind::Script:
						try_script();
						break;
					default:
						break;
					}
				}
				else {
					const std::string ext = to_lower_copy(asset_path.extension().string());
					if (ext == ".png" || ext == ".jpg" || ext == ".jpeg") {
						try_texture();
					}
					else if (ext == ".wav" || ext == ".mp3") {
						try_audio();
					}
					else if (ext == ".lua" || ext == ".json" || ext == ".txt") {
						try_script();
					}
				}

				if (!resolved.empty()) {
					return field.set(component, resolved);
				}
			}

			return false;
		}

		// Retrieve a live component instance from registry without hardcoding component types
		static entt::meta_any get_component_ref(entt::entity entity, const entt::meta_type& type) {
			auto get_ref = type.func("get_ref"_hs);
			if (!get_ref) {
				return {};
			}
			return get_ref.invoke({}, entity);
		}

		//Return live component instance from archtype without hardcoding component types.
		static entt::meta_any get_component_ref(std::shared_ptr<EngineFramework::Archtype> archtype_ptr, const entt::meta_type& type) {
			for (auto& comp : archtype_ptr->component_arr)
			{
				//Return the matching component.
				if (type != comp.type()) continue;
				return entt::forward_as_meta(comp);
			}
			return {};
		}

		struct EntityComponentEntry {
			entt::id_type type_id{};
			entt::meta_type meta{};
			entt::meta_any instance{};
		};

		// Walk through the entt::registry, keeps only those that contain the entity, resolve the meta type and fetches the component reference
		// Then, store the trio in a struct (id_type, meta_type, meta_any) defined above
		static std::vector<EntityComponentEntry> collect_components(entt::registry& reg, entt::entity entity) {
			std::vector<EntityComponentEntry> components{};
			for (auto&& [type_id, storage] : reg.storage()) {
				if (!storage.contains(entity)) {
					continue;
				}

				entt::meta_type meta = entt::resolve(storage.info());
				if (!meta) {
					continue;
				}

				entt::meta_any reference = get_component_ref(entity, meta);
				if (!reference) {
					continue;
				}

				components.emplace_back(EntityComponentEntry{
					type_id,
					meta,
					std::move(reference)
					});
			}
			return components;
		}

		

	} // namespace

	void DrawInspector(entt::registry& reg, entt::entity entity_selected, xresource::full_guid model_selected, bool is_display_entity) {

		//For scopeguard CALL_AT_END.
		using namespace EngineFramework;



		ImGui::Begin("Inspector");
		CALL_AT_END(imgui_callend, ImGui::End(););
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

		//No entity to display, no valid selected model to fallback to either.
		if (is_display_entity && (entity_selected == entt::null || !reg.valid(entity_selected))) {
			ImGui::TextDisabled("No entity selected.");
			return;
		}

		//Get the archtype if displaying model.		
		//Nullptr if displaying entity.
		std::shared_ptr<Archtype> archtype_ptr{};
		if (!is_display_entity)
		{
			archtype_ptr = AssetManagerSystem::GetResource<Archtype>(model_selected);
			/*
				ERROR!
				model_guid was verified earlier before calling this function (is_display_entity is false),
				but unable to get the archtype asset.
				Exit silently and hopefully the user can select another entity/model.
			*/
			if (!archtype_ptr)
			{
				LOGE("[InspectorPanel] DrawInspector Unable to find archtype asset, even though its guid is valid.");
				return;
			}
		}


		/*
			Display either the entity or the model based on is_display_entity.
			The entity or model id/ptr is guaranteed to be valid (based on checks done before this function is called).
		*/

		// --- Display Name ---
		std::string display_obj_name{};
		if (!is_display_entity) display_obj_name = AssetManagerSystem::GetName(model_selected);
		else display_obj_name = EngineFramework::GameObjectSystem::GetEntityName(entity_selected);
		ImGui::TextUnformatted(display_obj_name.c_str());
		ImGui::Separator();

		// --- Drag and Drop from AssetBrowser into Inspector ---
		// This allows automatic creation of components from say textures or audio, or drag drop into fields like guid.
		static std::string last_drop_feedback{};
		static float feedback_timer = 0.0f;

		EngineFramework::AssetWindow* asset_window_ptr = EngineFramework::AssetWindow::this_class;
		ComponentDropContext drop_context{};
		if (asset_window_ptr) {
			drop_context = build_drop_context(*asset_window_ptr);
		}

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(12.0f, 6.0f));
		bool add_component_clicked = ImGui::Button("Add Component##InspectorAddComponentDrop", ImVec2(-1.0f, 0.0f));
		ImGui::PopStyleVar();
		if (add_component_clicked) {
			ImGui::OpenPopup("InspectorAddComponentPopup");
		}

		if (!asset_window_ptr) {
			ImGui::TextDisabled("Asset window data unavailable. Enable the Asset Window to use drag-and-drop.");
		}

		if (asset_window_ptr && ImGui::BeginDragDropTarget()) {
			const ImGuiPayload* payload = nullptr;
			for (const std::string& type_str : drop_context.payload_types) {
				if (type_str.empty()) {
					continue;
				}
				if (const ImGuiPayload* pay = ImGui::AcceptDragDropPayload(type_str.c_str())) {
					payload = pay;
					break;
				}
			}

			if (payload && payload->Data) {
				const char* raw_path = static_cast<const char*>(payload->Data);
				std::string dropped_path_str{ raw_path };
				std::filesystem::path dropped_path = std::filesystem::path{ dropped_path_str }.lexically_normal();
				std::string extension = to_lower_copy(dropped_path.extension().string());

				std::unordered_set<std::string> newly_added_names;
				bool updated_any_field = false;

				if (!extension.empty()) {
					const auto match_it = drop_context.by_extension.find(extension);
					if (match_it != drop_context.by_extension.end()) {
						for (const ComponentDropMatch& match : match_it->second) {
							bool has_component = false;
							if (is_display_entity) { //entity
								if (auto* storage = reg.storage(match.type.id())) {
									has_component = storage->contains(entity_selected);
								}
							}
							else  //Model
							{
								//Check if any archtype component matches the same id.
								for (const entt::meta_any& archtype_comp : archtype_ptr->component_arr)
								{
									if (match.type == archtype_comp.type())
									{
										has_component = true;
										break;
									}
								}
							}

							if (!has_component) {
								entt::meta_any instance = match.type.construct();
								if (is_display_entity)
								{ //Entity, need to use attached emplace function to add to entity in registry.
									auto emplace = match.type.func("emplace"_hs);
									if (!emplace) {
										continue;
									}
									entt::meta_any result = emplace.invoke({}, entity_selected, static_cast<const void*>(&instance));
									if (!result || !result.cast<bool>()) {
										continue;
									}
								}
								else //Archtype, just add to array.
								{
									archtype_ptr->component_arr.push_back(instance);
								}
								newly_added_names.insert(match.component_name);
							}

							entt::meta_any component_ref;
							if (is_display_entity) component_ref = get_component_ref(entity_selected, match.type);
							else component_ref = get_component_ref(archtype_ptr, match.type);
							if (component_ref) {
								if (assign_field_from_path(component_ref, match.field, dropped_path)) {
									updated_any_field = true;
								}
							}
						}
					}
				}

				if (!extension.empty() && drop_context.by_extension.find(extension) == drop_context.by_extension.end()) {
					last_drop_feedback = "No registered component accepts files with extension '" + extension + "'.";
					feedback_timer = 3.0f;
				}
				else if (!newly_added_names.empty()) {
					std::vector<std::string> names_ordered(newly_added_names.begin(), newly_added_names.end());
					std::sort(names_ordered.begin(), names_ordered.end());
					std::string summary = "Added component";
					summary += names_ordered.size() > 1 ? "s: " : ": ";
					summary += names_ordered.front();
					for (size_t idx = 1; idx < names_ordered.size(); ++idx) {
						summary.append(", ");
						summary.append(names_ordered[idx]);
					}
					last_drop_feedback = std::move(summary);
					feedback_timer = 3.0f;
				}
				else if (updated_any_field) {
					last_drop_feedback = "Updated component fields from dropped asset.";
					feedback_timer = 3.0f;
				}
				else if (extension.empty()) {
					last_drop_feedback = "Dropped asset has no file extension; unable to match a component.";
					feedback_timer = 3.0f;
				}
				else {
					last_drop_feedback = "No changes applied from the dropped asset.";
					feedback_timer = 3.0f;
				}
			}

			ImGui::EndDragDropTarget();
		}

		// --- Adding Component ---
		if (ImGui::BeginPopup("InspectorAddComponentPopup")) {
			ImGui::TextUnformatted("Select a component to add");
			ImGui::Separator();
			static char add_component_filter[64] = "";
			ImGui::InputTextWithHint("##InspectorAddComponentSearch", "Search...", add_component_filter, IM_ARRAYSIZE(add_component_filter));
			ImGui::Separator();

			std::string filter_lower = to_lower_copy(add_component_filter);
			struct ComponentListEntry {
				entt::meta_type type;
				std::string name;
				std::string lower_name;
			};
			std::vector<ComponentListEntry> available_components{};

			for (const auto& [type_id, meta] : entt::resolve()) {
				if (!is_component_meta_type(meta)) {
					continue;
				}

				const char* meta_name = meta.name();
				if (!meta_name || meta_name[0] == '\0') {
					continue;
				}

				if (is_display_entity)
				{	//Entity
					auto* storage = reg.storage(type_id);
					if (storage && storage->contains(entity_selected)) {
						continue;
					}
				}
				else
				{
					//Model check if component is inside already.
					bool is_present{};
					for (auto& comp : archtype_ptr->component_arr)
					{
						if (meta == comp.type())
						{
							is_present = true;
							break;
						}
					}
					if (is_present) continue;
				}


				ComponentListEntry entry{};
				entry.type = meta;
				entry.name = meta_name;
				entry.lower_name = to_lower_copy(entry.name);
				available_components.emplace_back(std::move(entry));
			}

			std::sort(available_components.begin(), available_components.end(),
				[](const ComponentListEntry& lhs, const ComponentListEntry& rhs) {
					return lhs.lower_name < rhs.lower_name;
				});

			bool any_component_drawn = false;
			for (const ComponentListEntry& entry : available_components) {
				const std::string& display = entry.name;
				const std::string& lowered = entry.lower_name;
				const entt::meta_type& meta = entry.type;

				if (!filter_lower.empty() && lowered.find(filter_lower) == std::string::npos) {
					continue;
				}

				any_component_drawn = true;
				if (ImGui::Selectable(display.c_str())) {
					bool added = false;
					auto emplace = meta.func("emplace"_hs);
					if (emplace) {
						entt::meta_any instance = meta.construct();

						if (is_display_entity)
						{	//Entity
							entt::meta_any result = emplace.invoke({}, entity_selected, static_cast<const void*>(&instance));
							if (result && result.cast<bool>()) {
								added = true;
							}
						}
						else
						{	//Model
							archtype_ptr->component_arr.push_back(instance);
							added = true;
						}
					}

					if (added) {
						if (asset_window_ptr) {
							drop_context = build_drop_context(*asset_window_ptr);
						}
						last_drop_feedback = "Added component: " + display;
						feedback_timer = 3.0f;
						ImGui::CloseCurrentPopup();
					}
					else {
						last_drop_feedback = "Failed to add component: " + display;
						feedback_timer = 3.0f;
					}
				}
			}

			if (!any_component_drawn) {
				ImGui::TextDisabled("No components available.");
			}

			ImGui::EndPopup();
		}

		if (feedback_timer > 0.0f && !last_drop_feedback.empty()) {
			feedback_timer -= ImGui::GetIO().DeltaTime;
			ImGui::TextColored(ImVec4(0.6f, 0.85f, 0.6f, 1.0f), "%s", last_drop_feedback.c_str());
		}

		ImGui::Separator();

		// --- Draw Components and their Data in object ---
		std::vector<std::pair<entt::meta_type, std::string>> components_to_remove{};
		if (is_display_entity)
		{
			std::vector<EntityComponentEntry> components = collect_components(reg, entity_selected);
			for (auto& component : components) {
				const char* header = component.meta.name();
				if (!header || !*header) header = "Component";

				ImGui::PushID(reinterpret_cast<void*>(static_cast<uintptr_t>(component.type_id)));

				bool component_visible = true;
				if (ImGui::CollapsingHeader(header, &component_visible, ImGuiTreeNodeFlags_DefaultOpen)) {
					EditorFeatures::DrawMetaObject(component.meta, component.instance);
				}

				if (!component_visible) {
					components_to_remove.emplace_back(component.meta, std::string{ header });
				}

				ImGui::PopID();
			}
		}
		else
		{
			for (auto& comp : archtype_ptr->component_arr) {
				entt::meta_type meta = comp.type();
				const char* header = meta.name();
				if (!header || !*header) header = "Component";
				ImGui::PushID(reinterpret_cast<void*>(static_cast<uintptr_t>(meta.id())));

				bool component_visible = true;
				if (ImGui::CollapsingHeader(header, &component_visible, ImGuiTreeNodeFlags_DefaultOpen)) {
					EditorFeatures::DrawMetaObject(meta, comp);
				}

				if (!component_visible) {
					components_to_remove.emplace_back(meta, std::string{header});
				}

				ImGui::PopID();
			}
		}

		

		// To remove the components
		if (!components_to_remove.empty()) {
			for (const auto& [meta, name] : components_to_remove) {
				bool removed = false;
				if (is_display_entity)
				{
					auto remove = meta.func("remove"_hs);
					if (remove)
					{
						entt::meta_any result = remove.invoke({}, entity_selected);
						if (result && result.cast<bool>()) {
							removed = true;
						}
					}
				}
				else
				{	//Model
					for (auto it = archtype_ptr->component_arr.begin(); it != archtype_ptr->component_arr.end();)
					{
						if (it->type() != meta)
						{
							it++;
							continue;
						}
						archtype_ptr->component_arr.erase(it);
						break;
					}
				}

				if (removed) {
					last_drop_feedback = "Removed component: " + name;
				}
				else {
					last_drop_feedback = "Failed to remove component: " + name;
				}
				feedback_timer = 3.0f;
			}
		}

	}
}

#endif
