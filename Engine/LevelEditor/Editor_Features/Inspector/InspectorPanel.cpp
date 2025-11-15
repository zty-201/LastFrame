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
#include "HistorySystem.hpp"
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
#include <memory>

namespace EditorFeatures {

	namespace {

		using namespace entt::literals;
		using EngineFramework::HistorySystem;
		using EngineFramework::Record_Type;

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

		struct EntityComponentEntry {
			entt::id_type type_id{};
			entt::meta_type meta{};
			entt::meta_any instance{};
		};

		// Walk through the entt::registry, keeps only those that contain the entity, resolve the meta type and fetches the component reference
		// Then, store the trio in a struct (id_type, meta_type, meta_any) defined above
		static std::vector<EntityComponentEntry> collect_entity_components(entt::registry& reg, entt::entity entity) {
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

		static std::unique_ptr<EngineFramework::Record> start_component_history(entt::entity entity, const std::string& component_name, Record_Type type) {
			if (!HistorySystem::this_class) {
				return nullptr;
			}
			return HistorySystem::GetInstance().start_track(entity, component_name, type);
		}

		static void commit_component_history(std::unique_ptr<EngineFramework::Record>& record) {
			if (!record) {
				return;
			}
			HistorySystem::GetInstance().end_track(record);
		}

		static void record_component_added(entt::entity entity, const std::string& component_name) {
			auto record = start_component_history(entity, component_name, Record_Type::COMP_ADDED);
			commit_component_history(record);
		}

	} // namespace

	void DrawInspector(entt::registry& reg, entt::entity e) {
		ImGui::Begin("Inspector");

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

		std::string entity_name = EngineFramework::GameObjectSystem::GetEntityName(e);
		if (entity_name.empty()) {
			std::ostringstream label;
			label << "Entity 0x" << std::uppercase << std::setw(8) << std::setfill('0') << static_cast<unsigned>(entt::to_integral(e));
			entity_name = label.str();
		}

		ImGui::TextUnformatted(entity_name.c_str());
		ImGui::Separator();

		static std::string last_drop_feedback{};
		static float feedback_timer = 0.0f;

		EngineFramework::AssetWindow* asset_window_ptr = EngineFramework::AssetWindow::this_class;
		ComponentDropContext drop_context{};
		if (asset_window_ptr) {
			drop_context = build_drop_context(*asset_window_ptr);
		}

		// If history system singleton exists then create the two buttons for the undo/redo
		// Undo only enabled when it can be undo (under history singleton)
		// Redo only enabled when it can be redo (under history singleton also)
		if (HistorySystem::this_class) {
			HistorySystem& history = HistorySystem::GetInstance();
			ImGui::BeginGroup();
			ImGui::BeginDisabled(!history.can_undo());
			if (ImGui::Button("Undo##InspectorPanelUndo")) {
				history.undo();
			}
			ImGui::EndDisabled();
			ImGui::SameLine();
			ImGui::BeginDisabled(!history.can_redo());
			if (ImGui::Button("Redo##InspectorPanelRedo")) {
				history.redo();
			}
			ImGui::EndDisabled();
			ImGui::EndGroup();
			ImGui::Separator();
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
							const entt::id_type type_id = match.type.id();
							bool has_component = false;
							if (auto* storage = reg.storage(type_id)) {
								has_component = storage->contains(e);
							}

							if (!has_component) {
								entt::meta_any instance = match.type.construct();
								auto emplace = match.type.func("emplace"_hs);
								if (!emplace) {
									continue;
								}
								entt::meta_any result = emplace.invoke({}, e, static_cast<const void*>(&instance));
								if (!result || !result.cast<bool>()) {
									continue;
								}
								record_component_added(e, match.component_name);
								newly_added_names.insert(match.component_name);
							}

							if (entt::meta_any component_ref = get_component_ref(e, match.type)) {
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

				auto* storage = reg.storage(type_id);
				if (storage && storage->contains(e)) {
					continue;
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
						entt::meta_any result = emplace.invoke({}, e, static_cast<const void*>(&instance));
						if (result && result.cast<bool>()) {
							added = true;
						}
					}

					// If component is added - important for tracking here for undo purposes
					if (added) {
						record_component_added(e, display);

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

		// Actual draw
		std::vector<std::pair<entt::meta_type, std::string>> components_to_remove{};
		std::vector<EntityComponentEntry> components = collect_entity_components(reg, e);

		for (auto& component : components) {
			const char* header = component.meta.name();
			if (!header || !*header) header = "Component";

			ImGui::PushID(reinterpret_cast<void*>(static_cast<uintptr_t>(component.type_id)));

			bool component_visible = true;
			if (ImGui::CollapsingHeader(header, &component_visible, ImGuiTreeNodeFlags_DefaultOpen)) {
				EditorFeatures::DrawMetaObject(e, header, component.meta, component.instance);
			}

			if (!component_visible) {
				components_to_remove.emplace_back(component.meta, std::string{ header });
			}

			ImGui::PopID();
		}

		// To remove the components
		if (!components_to_remove.empty()) {
			for (const auto& [meta, name] : components_to_remove) {
				auto remove = meta.func("remove"_hs);
				bool removed = false;
				std::unique_ptr<EngineFramework::Record> removal_record = start_component_history(e, name, Record_Type::COMP_REMOVED);
				if (remove) {
					entt::meta_any result = remove.invoke({}, e);
					if (result && result.cast<bool>()) {
						removed = true;
						commit_component_history(removal_record);
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

		ImGui::End();
	}
}

#endif
