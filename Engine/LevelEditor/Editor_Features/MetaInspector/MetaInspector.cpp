/*****************************************************************/
/*!
\file   MetaInspector.cpp
\author Neo HuiZong
\par    neo.h\@digipen.edu
\date   October 2025
\brief
This file contains the definitions of the functions that are needed to
draw the controls and pushes the changes straight into live components
of the selected entity.
Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* *******************************************************************/
#if defined(PLATFORM_WINDOWS) && defined(_DEBUG)

#include "pch.hpp"
#include "MetaInspector.hpp"
#include "CameraComponent.h" // Testing purposes of property manager
#include "MeshComponent.h"
#include "AnimationComponent.h"
#include "LightComponent.h"
#include "TransformComponent.h"
#include "MaterialComponent.h"
#include "AssetManager.hpp"
#include "SoundAsset.hpp"
#include "scene.h"
#include "HistorySystem.hpp"
#include "AudioComponent.h"
#if defined(ENGINE_ENABLE_LUA)
#include "Scripting/Script.hpp"
#include "Scripting/ScriptAsset.hpp"
#endif
#include <unordered_map>
#include <unordered_set>
#include <string_view>
#include <imgui.h>
#include <array>
#include <iomanip>
#include <sstream>
#include <filesystem>
#include <cstring>
#include <memory>
#include <utility>

// FILE TO HANDLE THE META DATA OF THE ENTITIY
namespace EditorFeatures {

	// Add on to this map if want to hide any fields
	static const std::unordered_map<std::string_view, std::unordered_set<std::string_view>> hidden_fields = {
		{"CameraComponent", {"view_vector", "proj_matrix", "oldx", "oldy"}},
		{"MeshComponent", {"vao", "indexCount"}},
		{"TransformComponent", {"modelMatrix"}}// Hidden for now - testing purposes
	};

	static constexpr entt::id_type asset_picker_prop = entt::hashed_string{ "editor.asset_picker" }.value();

	static std::string to_lower(std::string s) {
		std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
		return s;
	}

	static std::unordered_map<ImGuiID, std::unique_ptr<EngineFramework::Record>> active_field_records;

	static void update_field_history(entt::entity entity, const char* component_name, const entt::meta_data& data, entt::meta_any& obj) {
		if (!EngineFramework::HistorySystem::this_class) {
			return;
		}
		const char* field_name = data.name();
		if (!field_name || field_name[0] == '\0') {
			return;
		}

		ImGuiID item_id = ImGui::GetItemID();
		if (item_id == 0) {
			return;
		}

		auto& history = EngineFramework::HistorySystem::GetInstance();

		if (ImGui::IsItemActivated()) {
			if (active_field_records.find(item_id) == active_field_records.end()) {
				const char* component_label = (component_name && component_name[0]) ? component_name : "";
				auto record = history.start_track(entity, component_label, field_name);
				if (record) {
					active_field_records.emplace(item_id, std::move(record));
				}
			}
		}

		const auto it = active_field_records.find(item_id);
		if (it == active_field_records.end()) {
			return;
		}

		if (ImGui::IsItemDeactivatedAfterEdit()) {
			entt::meta_any latest_value = data.get(obj);
			history.end_track(it->second, latest_value);
			active_field_records.erase(it);
		}
		else if (ImGui::IsItemDeactivated()) {
			active_field_records.erase(it);
		}
	}

	template<typename ApplyFunc>
	static void commit_instant_history(entt::entity entity, const char* component_name, const entt::meta_data& data, entt::meta_any& obj, ApplyFunc&& apply) {
		const char* field_name = data.name();
		if (!field_name || field_name[0] == '\0') {
			return;
		}

		const bool history_available = (EngineFramework::HistorySystem::this_class != nullptr);
		std::unique_ptr<EngineFramework::Record> record{};
		if (history_available) {
			auto& history = EngineFramework::HistorySystem::GetInstance();
			const char* component_label = (component_name && component_name[0]) ? component_name : "";
			record = history.start_track(entity, component_label, field_name);
			if (record && record->Type == EngineFramework::Record_Type::INVALID) {
				record.reset();
			}
		}

		apply();

		if (record) {
			auto& history = EngineFramework::HistorySystem::GetInstance();
			entt::meta_any latest_value = data.get(obj);
			history.end_track(record, latest_value);
		}
	}


	static std::string label_for(const entt::meta_data &data, entt::id_type id) {
		if (const char* n = data.name(); n && *n) {
			return std::string(n);
		}
		char buffer[64];
		std::snprintf(buffer, sizeof(buffer), "field_%u", static_cast<unsigned>(id));
		return std::string(buffer);
	}

	static inline void draw_vec3_drag(const char* label, glm::vec3& v, float speed = 0.05f) {
		float a[3]{ v.x, v.y, v.z };
		if (ImGui::DragFloat3(label, a, speed)) {
			v = { a[0], a[1], a[2] };
		}
	}

	static inline void draw_vec4_drag(const char* label, glm::vec4& v, float speed = 0.05f) {
		float a[4]{ v.x, v.y, v.z, v.w };
		if (ImGui::DragFloat4(label, a, speed)) {
			v = { a[0], a[1], a[2], a[3] };
		}
	}

	static std::string format_guid_hex(std::uint64_t value)
	{
		std::ostringstream oss;
		oss << std::uppercase << std::hex << std::setw(16) << std::setfill('0') << value;
		return oss.str();
	}

	static const char* payload_type_for_kind(AssetPickerKind kind)
	{
		switch (kind) {
		case AssetPickerKind::Texture: return "Textures";
		case AssetPickerKind::Audio:   return "Audio";
		case AssetPickerKind::Script:  return "Scripts";
		default: break;
		}
		return nullptr;
	}

	// Make the dropped file path into corresponding asset GUID
	static bool apply_payload_to_guid(xresource::full_guid& guid, AssetPickerKind kind, const std::filesystem::path& dropped_path)
	{
		using EngineFramework::AssetManagerSystem;

		// Bail out if no path
		if (dropped_path.empty()) {
			return false;
		}

		const std::string asset_name = dropped_path.stem().string();
		if (asset_name.empty()) {
			return false;
		}

		// Specifically, check for texture texture, audio and script
		switch (kind) {
		case AssetPickerKind::Texture: {
			auto candidate = AssetManagerSystem::GetFullGuid(AssetManagerSystem::TEXTURE_GUID, asset_name);
			if (candidate.empty()) {
				candidate = AssetManagerSystem::GetFullGuid(AssetManagerSystem::CUBEMAP_GUID, asset_name);
			}
			if (!candidate.empty()) {
				guid = candidate;
				return true;
			}
			break;
		}
		case AssetPickerKind::Audio: {
			auto candidate = AssetManagerSystem::GetFullGuid(AssetManagerSystem::AUDIO_GUID, asset_name);
			if (!candidate.empty()) {
				guid = candidate;
				return true;
			}
			break;
		}
		case AssetPickerKind::Script: {
			auto candidate = AssetManagerSystem::GetFullGuid(AssetManagerSystem::SCRIPT_GUID, asset_name);
			if (!candidate.empty()) {
				guid = candidate;
				return true;
			}
			break;
		}
		default:
			break;
		}

		return false;
	}

	// To handle the guid dropped from asset browser based on their tags
	static bool handle_asset_guid_drop(xresource::full_guid& guid, AssetPickerKind kind)
	{
		const char* payload_type = payload_type_for_kind(kind);

		bool changed = false;
		if (ImGui::BeginDragDropTarget()) {
			const char* candidates[4] = {};
			int candidate_count = 0;
			auto add_candidate = [&](const char* type) {
				if (!type) {
					return;
				}
				for (int idx = 0; idx < candidate_count; ++idx) {
					if (std::strcmp(candidates[idx], type) == 0) {
						return;
					}
				}
				candidates[candidate_count++] = type;
			};

			add_candidate(payload_type);
			add_candidate("GENERAL");

			const ImGuiPayload* payload = nullptr;
			for (int idx = 0; idx < candidate_count && !payload; ++idx) {
				payload = ImGui::AcceptDragDropPayload(candidates[idx]);
			}

			if (payload && payload->Data && payload->DataSize > 0) {
				const char* raw = static_cast<const char*>(payload->Data);
				std::string dropped(raw, raw + payload->DataSize - 1);
				std::filesystem::path dropped_path = std::filesystem::path{ dropped }.lexically_normal();
				if (apply_payload_to_guid(guid, kind, dropped_path)) {
					changed = true;
				}
			}
			ImGui::EndDragDropTarget();
		}
		return changed;
	}

	static std::filesystem::path make_relative_to_assets(const std::filesystem::path& input_path)
	{
		if (input_path.empty()) {
			return {};
		}

		std::error_code ec_root{};
		std::error_code ec_path{};

		const std::filesystem::path root = EngineFramework::AssetManagerSystem::GetAssetFolderPrefix();
		const std::filesystem::path abs_root = std::filesystem::absolute(root, ec_root);
		const std::filesystem::path abs_path = std::filesystem::absolute(input_path, ec_path);

		if (!ec_root && !ec_path) {
			std::error_code ec_rel{};
			std::filesystem::path relative = std::filesystem::relative(abs_path, abs_root, ec_rel);
			if (!ec_rel && !relative.empty()) {
				return relative.lexically_normal();
			}
		}

		return input_path.lexically_normal();
	}

	static std::vector<std::filesystem::path> collect_script_paths()
	{
		std::vector<std::filesystem::path> scripts{};

		const std::filesystem::path root = EngineFramework::AssetManagerSystem::GetAssetFolderPrefix();
		const std::filesystem::path scripts_dir = root / "Extern/Scripts";

		std::error_code ec{};
		if (!std::filesystem::exists(scripts_dir, ec)) {
			return scripts;
		}

		std::filesystem::recursive_directory_iterator it{ scripts_dir, std::filesystem::directory_options::skip_permission_denied, ec };
		std::filesystem::recursive_directory_iterator end{};

		for (; it != end && !ec; it.increment(ec)) {
			if (ec) {
				break;
			}

			const auto& entry = *it;
			if (!entry.is_regular_file()) {
				continue;
			}

			const std::string ext = to_lower(entry.path().extension().string());
			if (ext != ".lua") {
				continue;
			}

			std::filesystem::path relative = make_relative_to_assets(entry.path());
			if (!relative.empty()) {
				scripts.emplace_back(std::move(relative));
			}
		}

		std::sort(scripts.begin(), scripts.end(), [](const std::filesystem::path& a, const std::filesystem::path& b) {
			return a.generic_string() < b.generic_string();
			});
		scripts.erase(std::unique(scripts.begin(), scripts.end(), [](const std::filesystem::path& a, const std::filesystem::path& b) {
			return a.generic_string() == b.generic_string();
			}), scripts.end());

		return scripts;
	}

	static AssetPickerDescriptor make_asset_picker_descriptor(AssetPickerKind kind);
	static bool draw_asset_guid_picker(const char* label, xresource::full_guid& guid, AssetPickerKind kind, std::string_view none_label, const std::vector<AssetPickerOptions>& sources, AssetDisplayFormatter formatter = {});

#if defined(ENGINE_ENABLE_LUA)
	static bool accept_script_payload(xresource::instance_guid& guid) {
		const char* payload_types[] = { "Scripts", "GENERAL" };
		for (const char* type : payload_types) {
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(type)) {
				if (payload && payload->Data && payload->DataSize > 0) {
					const char* raw = static_cast<const char*>(payload->Data);
					std::string dropped(raw, raw + payload->DataSize - 1);
					std::filesystem::path dropped_path = std::filesystem::path{ dropped }.lexically_normal();
					xresource::full_guid resolved{};
					if (apply_payload_to_guid(resolved, AssetPickerKind::Script, dropped_path)) {
						guid = resolved.m_Instance;
						return true;
					}
				}
			}
		}
		return false;
	}

	static void draw_lua_script_component(entt::entity entity, const char* component_name, entt::meta_any& obj) {
		auto& component = obj.cast<Engine::LuaScriptComponent&>();
		const char* component_label = (component_name && component_name[0]) ? component_name : "LuaScriptComponent";

		const entt::meta_type component_meta = entt::resolve<Engine::LuaScriptComponent>();
		const entt::meta_data enabled_data = component_meta.data(entt::hashed_string{ "Enabled" }.value());
		const entt::meta_data scripts_data = component_meta.data(entt::hashed_string{ "Scripts" }.value());

		auto apply_enabled_edit = [&](auto&& fn) {
			if (enabled_data) {
				commit_instant_history(entity, component_label, enabled_data, obj, [&]() {
					fn();
					enabled_data.set(obj, component.Enabled);
				});
			}
			else {
				fn();
			}
		};

		auto apply_scripts_edit = [&](auto&& fn) {
			if (scripts_data) {
				commit_instant_history(entity, component_label, scripts_data, obj, [&]() {
					fn();
					scripts_data.set(obj, component.Scripts);
				});
			}
			else {
				fn();
			}
		};

		bool enabled = component.Enabled;
		if (ImGui::Checkbox("Enabled", &enabled)) {
			apply_enabled_edit([&]() {
				component.Enabled = enabled;
			});
		}
		ImGui::Spacing();

		const auto descriptor = make_asset_picker_descriptor(AssetPickerKind::Script);
		const bool has_sources = !descriptor.sources.empty();

		std::vector<std::size_t> to_remove{};
		to_remove.reserve(component.Scripts.size());

		if (ImGui::BeginTable("LuaScriptsTable", 3, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_NoSavedSettings)) {
			ImGui::TableSetupColumn("Slot", ImGuiTableColumnFlags_WidthFixed, 90.0f);
			ImGui::TableSetupColumn("Script", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthFixed, 80.0f);

			if (component.Scripts.empty()) {
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::TextDisabled("-");
				ImGui::TableSetColumnIndex(1);
				ImGui::TextDisabled("No scripts attached.");
				ImGui::TableSetColumnIndex(2);
				ImGui::Dummy(ImVec2(0.0f, 0.0f));
			}
			else {
				for (std::size_t idx = 0; idx < component.Scripts.size(); ++idx) {
					auto& inst = component.Scripts[idx];

					ImGui::TableNextRow();

					ImGui::TableSetColumnIndex(0);
					ImGui::Text("Script %zu", idx + 1);
					const ImVec4 status_color = inst.Loaded ? ImVec4(0.45f, 0.8f, 0.45f, 1.0f) : ImVec4(0.86f, 0.45f, 0.45f, 1.0f);
					ImGui::TextColored(status_color, inst.Loaded ? "Loaded" : "Pending");

					ImGui::PushID(static_cast<int>(idx));

					ImGui::TableSetColumnIndex(1);
					if (has_sources) {
						xresource::full_guid full{ inst.ScriptGuid, EngineFramework::AssetManagerSystem::SCRIPT_GUID };
						std::string picker_label = "##LuaScriptPicker" + std::to_string(idx);
						if (draw_asset_guid_picker(picker_label.c_str(), full, AssetPickerKind::Script, descriptor.none_label, descriptor.sources, descriptor.formatter)) {
							if (inst.ScriptGuid != full.m_Instance) {
								const xresource::instance_guid new_guid = full.m_Instance;
								apply_scripts_edit([&]() {
									inst.ScriptGuid = new_guid;
									inst.Loaded = false;
								});
							}
						}
					}
					else {
						ImGui::TextDisabled("No script assets available.");
					}

					ImGui::TableSetColumnIndex(2);
					if (ImGui::SmallButton("Remove")) {
						to_remove.push_back(idx);
					}

					ImGui::PopID();
				}
			}

			ImGui::EndTable();
		}

		for (auto it = to_remove.rbegin(); it != to_remove.rend(); ++it) {
			const std::size_t remove_index = *it;
			apply_scripts_edit([&]() {
				component.Scripts.erase(component.Scripts.begin() + static_cast<long long>(remove_index));
			});
		}

		if (ImGui::Button("Add Script")) {
			apply_scripts_edit([&]() {
				auto& inst = component.Scripts.emplace_back();
				inst.Loaded = false;
			});
		}

		ImGui::Spacing();

		ImVec2 drop_size(ImGui::GetContentRegionAvail().x, ImGui::GetFrameHeight() * 2.0f);
		if (drop_size.x <= 0.0f) {
			drop_size.x = 200.0f;
		}
		if (drop_size.y <= 0.0f) {
			drop_size.y = ImGui::GetFrameHeight() * 2.0f;
		}

		ImGui::PushID("LuaScriptDropZone");
		if (ImGui::BeginChild("DropZone", drop_size, true, ImGuiWindowFlags_NoScrollbar)) {
			ImGui::TextWrapped("Drag and drop Lua script assets here to attach them.");
			if (ImGui::BeginDragDropTarget()) {
				xresource::instance_guid new_guid{};
				if (accept_script_payload(new_guid)) {
					const bool already_present = std::any_of(component.Scripts.begin(), component.Scripts.end(),
						[&](const Engine::LuaScriptComponent::LuaInstance& existing) {
							return existing.ScriptGuid == new_guid;
						});
					if (!already_present) {
						apply_scripts_edit([&]() {
							Engine::LuaScriptComponent::LuaInstance inst{};
							inst.ScriptGuid = new_guid;
							inst.Loaded = false;
							component.Scripts.emplace_back(std::move(inst));
						});
					}
				}
				ImGui::EndDragDropTarget();
			}
		}
		ImGui::EndChild();
		ImGui::PopID();
	}
#endif

	template<typename T>
	static std::vector<std::pair<xresource::instance_guid, std::string>> build_asset_entries(const std::vector<std::pair<xresource::full_guid, std::shared_ptr<T>>>& list) {
		std::vector<std::pair<xresource::instance_guid, std::string>> out;
		out.reserve(list.size());
		for (const auto& [guid, _] : list) {
			std::string name = EngineFramework::AssetManagerSystem::GetName(guid);
			if (name.empty()) {
				name = format_guid_hex(guid.m_Instance.m_Value);
			}
			out.emplace_back(guid.m_Instance, std::move(name));
		}
		return out;
	}

	static AssetPickerDescriptor make_asset_picker_descriptor(AssetPickerKind kind) {
		using EngineFramework::AssetManagerSystem;

		switch (kind) {
		case AssetPickerKind::Texture: {
			using SceneTexture = Scene::Texture;
			auto textures =
				build_asset_entries(AssetManagerSystem::GetAllResourceOfType<SceneTexture>(
					AssetManagerSystem::TEXTURE_GUID));
			auto cubemaps =
				build_asset_entries(AssetManagerSystem::GetAllResourceOfType<SceneTexture>(
					AssetManagerSystem::CUBEMAP_GUID));
			return {
				"None",
				{
					{ AssetManagerSystem::TEXTURE_GUID, {}, std::move(textures) },
					{ AssetManagerSystem::CUBEMAP_GUID, "(Cubemap)", std::move(cubemaps) }
				},
				[](const xresource::full_guid&, std::string display, std::string_view suffix) {
					if (!suffix.empty()) {
						display += " ";
						display.append(suffix);
					}
					return display;
				}
			};
		}
		case AssetPickerKind::Audio: {
			auto sounds =
				build_asset_entries(AssetManagerSystem::GetAllResourceOfType<SoundAsset>(
					AssetManagerSystem::AUDIO_GUID));
			return { "No Audio", { { AssetManagerSystem::AUDIO_GUID, {}, std::move(sounds) } }, {} };
		}
#if defined(ENGINE_ENABLE_LUA)
		case AssetPickerKind::Script: {
			auto scripts =
				build_asset_entries(AssetManagerSystem::GetAllResourceOfType<ScriptAsset>(
					AssetManagerSystem::SCRIPT_GUID));
			return { "No Script", { { AssetManagerSystem::SCRIPT_GUID, {}, std::move(scripts) } }, {} };
		}
#else
		case AssetPickerKind::Script: {
			return { "No Script", {}, {} };
		}
#endif
		}
		return { "None", {}, {} };
	}

	// To render the inspector widget used for asset-back fields
	static bool draw_asset_guid_picker(const char* label, xresource::full_guid& guid, AssetPickerKind kind, std::string_view none_label, const std::vector<AssetPickerOptions>& sources, AssetDisplayFormatter formatter) {
		using EngineFramework::AssetManagerSystem;

		std::string current;

		// If GUID is empty, display none
		// If not look through the asset manager system to get the current guid
		if (guid.empty()) {
			current = std::string{ none_label };
		}
		else {
			current = AssetManagerSystem::GetName(guid);
			if (current.empty()) {
				current = format_guid_hex(guid.m_Instance.m_Value);
			}
			if (formatter) {
				std::string_view suffix{};
				for (const auto& source : sources) {
					if (source.type_guid == guid.m_Type) {
						suffix = source.suffix;
						break;
					}
				}
				current = formatter(guid, std::move(current), suffix);
			}
		}

		bool changed = false;

		if (ImGui::BeginCombo(label, current.c_str())) {
				if (ImGui::Selectable(none_label.data(), guid.empty())) {
					guid.clear();
					changed = true;
				}

				for (const auto& source : sources) {
					for (const auto& [instance, display] : source.entries) {
						const xresource::full_guid candidate{ instance, source.type_guid };
						std::string row_label = display;
						if (formatter) {
							row_label = formatter(candidate, std::move(row_label), source.suffix);
						}
						else if (!source.suffix.empty()) {
							row_label += " ";
							row_label.append(source.suffix);
						}
						const bool selected = (!guid.empty() && guid == candidate);
						if (ImGui::Selectable(row_label.c_str(), selected)) {
							guid = candidate;
							changed = true;
						}
						if (selected) {
							ImGui::SetItemDefaultFocus();
						}
					}
				}
				ImGui::EndCombo();
			}
			
		// To accept a payload matching either the component's specialized type (currently is texture, audio, scripts - can add on further uh) or general payload from the asset browser
		// Resolves the asset name, set GUID
			if (handle_asset_guid_drop(guid, kind)) {
				changed = true;
			}

			return changed;
		}

	//static bool draw_texture_guid_picker(const char* label, xresource::full_guid& guid, const char* none_label = "None") {
	//	using EngineFramework::AssetManagerSystem;
	//	using SceneTexture = Scene::Texture;

	//	auto build_entries = [](const auto& list) {
	//		std::vector<std::pair<xresource::instance_guid, std::string>> out;
	//		out.reserve(list.size());
	//		for (const auto& [instance, _] : list) {
	//			std::string name = AssetManagerSystem::GetName(instance);
	//			if (name.empty()) {
	//				name = format_guid_hex(instance.m_Value);
	//			}
	//			out.emplace_back(instance, std::move(name));
	//		}
	//		return out;
	//		};

	//	std::vector<AssetPickerOptions> sources = {
	//		{ AssetManagerSystem::TEXTURE_GUID,
	//		  {},
	//		  build_entries(AssetManagerSystem::GetAllResourceOfType<SceneTexture>(
	//			  AssetManagerSystem::TEXTURE_GUID)) },
	//		{ AssetManagerSystem::CUBEMAP_GUID,
	//		  "(Cubemap)",
	//		  build_entries(AssetManagerSystem::GetAllResourceOfType<SceneTexture>(
	//			  AssetManagerSystem::CUBEMAP_GUID)) }
	//	};

	//	auto formatter = [](const xresource::full_guid& candidate,
	//		std::string display,
	//		std::string_view suffix) {
	//			if (!suffix.empty()) {
	//				display += " ";
	//				display.append(suffix);
	//			}
	//			return display;
	//		};

	//	return draw_asset_guid_picker(label, guid, none_label, sources, formatter);
	//}

	//static std::string describe_guid_name(const xresource::full_guid& guid, std::string display)
	//{
	//	if (guid.m_Type == EngineFramework::AssetManagerSystem::TEXTURE_GUID) {
	//		return display;
	//	}
	//	if (guid.m_Type == EngineFramework::AssetManagerSystem::CUBEMAP_GUID) {
	//		return display + " (Cubemap)";
	//	}
	//	return display + " (Unknown)";
	//}

	//static bool draw_texture_guid_picker(const char* label, xresource::full_guid& guid, const char* none_label = "None")
	//{
	//	using EngineFramework::AssetManagerSystem;
	//	using SceneTexture = Scene::Texture;

	//	const auto textures = AssetManagerSystem::GetAllResourceOfType<SceneTexture>(AssetManagerSystem::TEXTURE_GUID);
	//	const auto cubemaps = AssetManagerSystem::GetAllResourceOfType<SceneTexture>(AssetManagerSystem::CUBEMAP_GUID);

	//	std::string current = guid.empty() ? none_label : AssetManagerSystem::GetName(guid.m_Instance);
	//	if (current.empty()) {
	//		current = format_guid_hex(guid.m_Instance.m_Value);
	//	}
	//	if (!guid.empty()) {
	//		current = describe_guid_name(guid, current);
	//	}

	//	bool changed = false;
	//	if (ImGui::BeginCombo(label, current.c_str())) {
	//		if (ImGui::Selectable(none_label, guid.empty())) {
	//			guid.clear();
	//			changed = true;
	//		}

	//		auto draw_entries = [&](const auto& list, const xresource::type_guid& type, std::string_view suffix) {
	//			for (const auto& [instance, _] : list) {
	//				std::string name = AssetManagerSystem::GetName(instance);
	//				if (name.empty()) {
	//					name = format_guid_hex(instance.m_Value);
	//				}
	//				const xresource::full_guid candidate{ instance, type };
	//				std::string label_text = suffix.empty() ? name : (name + " " + std::string{ suffix });
	//				const bool selected = (!guid.empty() && guid == candidate);
	//				if (ImGui::Selectable(label_text.c_str(), selected)) {
	//					guid = candidate;
	//					changed = true;
	//				}
	//				if (selected) {
	//					ImGui::SetItemDefaultFocus();
	//				}
	//			}
	//		};

	//		draw_entries(textures, AssetManagerSystem::TEXTURE_GUID, {});
	//		draw_entries(cubemaps, AssetManagerSystem::CUBEMAP_GUID, "(Cubemap)");
	//		ImGui::EndCombo();
	//	}
	//	return changed;
	//}
	
	// Main function to view/edit property
	// Takes a mneta type and the corresponding instance and writes the edited values back through entt::meta_data::set
	void DrawMetaObject(entt::entity entity, const char* component_name, entt::meta_type& mt, entt::meta_any& obj) {
		//const char* typeName = mt.name();

		auto t = obj.type();

#if defined(ENGINE_ENABLE_LUA)
		if (mt == entt::resolve<Engine::LuaScriptComponent>()) {
			if (obj.allow_cast<Engine::LuaScriptComponent&>()) {
				draw_lua_script_component(entity, component_name, obj);
			}
			return;
		}
#endif

		// ------- CUSTOM UI FOR AUDIO COMPONENT -------
		if (mt == entt::resolve<AudioComponent>())
		{
			auto& comp = obj.cast<AudioComponent&>();

			using EF_AM = EngineFramework::AssetManagerSystem;
			using EF_AUD = EngineFramework::AudioSystem;

			// comp.audio_guid is already full guid
			xresource::full_guid full = comp.audio_guid;

			// Load audio
			std::shared_ptr<SoundAsset> sound = nullptr;
			if (!full.empty())
				sound = EF_AM::GetResource<SoundAsset>(full);

			//---------------------
			// Volume (clamped)
			//---------------------
			comp.volume = std::clamp(comp.volume, 0.0f, 1.0f);

			if (ImGui::SliderFloat("Volume", &comp.volume, 0.0f, 1.0f))
			{
				// If audio is playing, update volume in real time here if your AudioSystem supports it
			}

			//---------------------
			// Buttons
			//---------------------
			if (ImGui::Button("Play"))
			{
				if (sound)
				{
					EF_AUD::GetInstance().Play(sound, comp.volume, comp.loop,
						EngineFramework::AudioGroup::Music);
				}
			}

			ImGui::SameLine();
			if (ImGui::Button("Stop"))
			{
				EF_AUD::GetInstance().StopAll();
			}

			ImGui::SameLine();
			if (ImGui::Button("Pause"))
			{
				EF_AUD::GetInstance().PauseAll(true);
			}

			ImGui::Separator();
		}
		
		// Each entry is pair<entt::id_type, entt::meta_data> - for self reference
		for (auto&& pair : mt.data()) {
			const entt::id_type id = pair.first;
			const entt::meta_data data = pair.second;
			entt::meta_any field = obj.get(pair.first);

			const std::string label = label_for(data, id);
			const char* type_name = mt.name();
			const std::string_view type_key = (type_name && *type_name) ? std::string_view{ type_name } : std::string_view{};

			if (const auto hidden = hidden_fields.find(type_key);
				hidden != hidden_fields.end() && hidden->second.count(label)) {
				continue;
			}

			ImGui::PushID(reinterpret_cast<void*>(static_cast<uintptr_t>(id)));


			// Skip the hidden fields

			if (field.type() == entt::resolve<int>()) {
				int& v = field.cast<int&>();
				const bool interacted = ImGui::DragInt(label.c_str(), &v, 1.0f);
				update_field_history(entity, component_name, data, obj);
				if (interacted) {
					data.set(obj, v);
				}
			}
			else if (field.type().is_enum()) {
				entt::meta_any tmp = field;                  // copy keeps original untouched
				if (!tmp.allow_cast<int>()) {
					ImGui::TextDisabled("%s (enum, no int conversion)", label.c_str());
					ImGui::PopID();
					continue;
				}

				// For camera type enum
				if (field.type() == entt::resolve<CameraComponent::CameraType>()) {
					auto& value = field.cast<CameraComponent::CameraType&>();
					int current = static_cast<int>(value);

					if (ImGui::RadioButton("Orbiting", current == static_cast<int>(CameraComponent::CameraType::ORBITING))) {
						current = static_cast<int>(CameraComponent::CameraType::ORBITING);
					}
					ImGui::SameLine();
					if (ImGui::RadioButton("Walking", current == static_cast<int>(CameraComponent::CameraType::WALKING))) {
						current = static_cast<int>(CameraComponent::CameraType::WALKING);
					}

					if (static_cast<int>(value) != current) {
						const auto new_value = static_cast<CameraComponent::CameraType>(current);
						commit_instant_history(entity, component_name, data, obj, [&]() {
							value = new_value;
							data.set(obj, value);
						});
					}
				}

				else if (field.type() == entt::resolve<LightComponent::Type>()) {
					auto& value = field.cast<LightComponent::Type&>();
					int current = static_cast<int>(value);

					if (ImGui::RadioButton("Directional", current == static_cast<int>(LightComponent::Type::DIRECTIONAL))) {
						current = static_cast<int>(LightComponent::Type::DIRECTIONAL);
					}
					ImGui::SameLine();
					if (ImGui::RadioButton("Point", current == static_cast<int>(LightComponent::Type::POINT))) {
						current = static_cast<int>(LightComponent::Type::POINT);
					}
					ImGui::SameLine();
					if (ImGui::RadioButton("Spot", current == static_cast<int>(LightComponent::Type::SPOT))) {
						current = static_cast<int>(LightComponent::Type::SPOT);
					}

					if (static_cast<int>(value) != current) {
						const auto new_value = static_cast<LightComponent::Type>(current);
						commit_instant_history(entity, component_name, data, obj, [&]() {
							value = new_value;
							data.set(obj, value);
							});
					}
				}
				// Temp test
				//int raw = tmp.cast<int>();
				//if (ImGui::DragInt(label.c_str(), &raw, 1.f)) {
				//	if (auto converted = field.type().construct(raw); converted) {
				//		data.set(obj, converted);
				//	}
				//}
			}

			// To test camera component enums - if works, can do the same for other components I guess ah
			else if (field.type() == entt::resolve<CameraComponent::CameraType>()) {
				auto& value = field.cast<CameraComponent::CameraType&>();
				int current = static_cast<int>(value);
				constexpr const char* options[] = { "Orbiting", "Walking" };

				if (ImGui::Combo(label.c_str(), &current, options, IM_ARRAYSIZE(options))) {
					const auto new_value = static_cast<CameraComponent::CameraType>(current);
					commit_instant_history(entity, component_name, data, obj, [&]() {
						value = new_value;
						data.set(obj, value);
					});
				}
			}

			else if (field.type() == entt::resolve<float>()) {
				float& v = field.cast<float&>();
				const bool interacted = ImGui::DragFloat(label.c_str(), &v, 0.01f);
				update_field_history(entity, component_name, data, obj);
				if (interacted) {
					data.set(obj, v);
				}
			}

			else if (field.type() == entt::resolve<double>()) {
				double& v = field.cast<double&>();
				const bool interacted = ImGui::InputDouble(label.c_str(), &v);
				update_field_history(entity, component_name, data, obj);
				if (interacted) {
					data.set(obj, v);
				}
			}

			else if (field.type() == entt::resolve<bool>()) {
				bool& v = field.cast<bool&>();
				if (ImGui::Checkbox(label.c_str(), &v)) {
					commit_instant_history(entity, component_name, data, obj, [&]() {
						data.set(obj, v);
					});
				}
			}

			else if (field.type() == entt::resolve<std::string>()) {
				std::string& s = field.cast<std::string&>();
				char buffer[512];
				std::snprintf(buffer, sizeof(buffer), "%s", s.c_str());
				const bool interacted = ImGui::InputText(label.c_str(), buffer, sizeof(buffer));
				update_field_history(entity, component_name, data, obj);
				if (interacted) {
					data.set(obj, std::string{ buffer });
				}
			}

			else if (field.type() == entt::resolve<std::filesystem::path>()) {
				auto& path = field.cast<std::filesystem::path&>();
				const auto* picker = static_cast<const AssetPickerTag*>(data.custom());
				const bool is_script_field = picker && picker->kind == AssetPickerKind::Script;

				if (is_script_field) {
					std::filesystem::path normalized = path.lexically_normal();
					std::string preview = normalized.filename().string();
					if (preview.empty()) {
						preview = normalized.empty() ? "No Script" : normalized.generic_string();
					}

					const std::string combo_preview = preview.empty() ? "No Script" : preview;
					const std::string tooltip_text = normalized.empty() ? std::string{} : normalized.generic_string();

					if (ImGui::BeginCombo(label.c_str(), combo_preview.c_str())) {
						const bool is_none_selected = normalized.empty();
						if (ImGui::Selectable("No Script", is_none_selected)) {
							if (!is_none_selected) {
								commit_instant_history(entity, component_name, data, obj, [&]() {
									path.clear();
									data.set(obj, path);
									normalized.clear();
								});
							}
						}

						const std::vector<std::filesystem::path> scripts = collect_script_paths();
						const std::string current_value = normalized.generic_string();
						for (const auto& script_rel : scripts) {
							const std::string script_rel_str = script_rel.generic_string();
							std::string row_label = script_rel.filename().string();
							if (row_label.empty()) {
								row_label = script_rel_str;
							}

							const bool selected = (!current_value.empty() && current_value == script_rel_str);
							if (ImGui::Selectable(row_label.c_str(), selected)) {
								if (!selected) {
									commit_instant_history(entity, component_name, data, obj, [&]() {
										path = script_rel;
										path = path.lexically_normal();
										data.set(obj, path);
										normalized = path;
									});
								}
							}
							if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort) && !script_rel_str.empty()) {
								ImGui::SetTooltip("%s", script_rel_str.c_str());
							}
							if (selected) {
								ImGui::SetItemDefaultFocus();
							}
						}
						ImGui::EndCombo();
					}

					if (!tooltip_text.empty() && ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
						ImGui::SetTooltip("%s", tooltip_text.c_str());
					}

					if (ImGui::BeginDragDropTarget()) {
						const char* payload_types[] = { "Scripts", "GENERAL" };
						const ImGuiPayload* payload = nullptr;
						for (const char* type : payload_types) {
							if (!payload) {
								payload = ImGui::AcceptDragDropPayload(type);
							}
						}
						if (payload && payload->Data && payload->DataSize > 0) {
							const char* raw = static_cast<const char*>(payload->Data);
							std::string dropped(raw, raw + payload->DataSize - 1);
							std::filesystem::path dropped_path = std::filesystem::path{ dropped }.lexically_normal();
							const std::string ext = to_lower(dropped_path.extension().string());
							if (ext == ".lua") {
								std::filesystem::path relative = make_relative_to_assets(dropped_path);
								if (!relative.empty()) {
									commit_instant_history(entity, component_name, data, obj, [&]() {
										path = relative.lexically_normal();
										data.set(obj, path);
										normalized = path;
									});
								}
							}
						}
						ImGui::EndDragDropTarget();
					}
				}
				else {
				std::string current = path.generic_string();
				char buffer[512];
				std::snprintf(buffer, sizeof(buffer), "%s", current.c_str());
				const bool interacted = ImGui::InputText(label.c_str(), buffer, sizeof(buffer));
				update_field_history(entity, component_name, data, obj);
				if (interacted) {
					path = std::filesystem::path{ buffer };
					data.set(obj, path);
				}
			}
			}

			else if (field.type() == entt::resolve<glm::vec3>()) {
				glm::vec3& v = field.cast<glm::vec3&>();

				// Heuristic color picker
				std::string lower_name = to_lower(label);
				bool colorish = (lower_name.find("color") != std::string::npos) || (lower_name.find("albedo") != std::string::npos) || (lower_name.find("tint") != std::string::npos);

				if (colorish) {
					float color[3]{ v.x, v.y, v.z };
					const bool interacted = ImGui::ColorEdit3(label.c_str(), color);
					update_field_history(entity, component_name, data, obj);
					if (interacted) {
						v = { color[0], color[1], color[2] };
						data.set(obj, v);
					}
				}
				else {
					draw_vec3_drag(label.c_str(), v);
					update_field_history(entity, component_name, data, obj);
					data.set(obj, v);
				}
			}
			else if (field.type() == entt::resolve<glm::vec4>()) {
				glm::vec4& v = field.cast<glm::vec4&>();

				std::string lower_name = to_lower(label);
				bool colorish = (lower_name.find("color") != std::string::npos) || (lower_name.find("albedo") != std::string::npos) || (lower_name.find("tint") != std::string::npos);

				if (colorish) {
					float color[4]{ v.x, v.y, v.z, v.w };
					const bool interacted = ImGui::ColorEdit4(label.c_str(), color);
					update_field_history(entity, component_name, data, obj);
					if (interacted) {
						v = { color[0], color[1], color[2], color[3] };
						data.set(obj, v);
					}
				}
				else {
					draw_vec4_drag(label.c_str(), v);
					update_field_history(entity, component_name, data, obj);
					data.set(obj, v);
				}
			}

			else if (field.type() == entt::resolve<xresource::full_guid>()) {
				auto& guid = field.cast<xresource::full_guid&>();
					if (const auto tag = static_cast<const AssetPickerTag*>(data.custom())) {
						const auto descriptor = make_asset_picker_descriptor(tag->kind);
						if (!descriptor.sources.empty() && draw_asset_guid_picker(label.c_str(), guid, tag->kind, descriptor.none_label, descriptor.sources, descriptor.formatter)) {
							commit_instant_history(entity, component_name, data, obj, [&]() {
								data.set(obj, guid);
							});
						}
					}
				else {
					ImGui::TextDisabled("%s (full_guid editor not implemented)", label.c_str());
				}
			}

			else if (field.type().is_sequence_container())
			{
				if (ImGui::TreeNode(label.c_str()))
				{
					auto view = field.as_sequence_container();
					int index = 0;
					for (auto it = view.begin(); it != view.end(); ++it)
					{
						ImGui::PushID(index);

						entt::meta_any element = *it;
						std::string element_label = "Element " + std::to_string(index);

						if (ImGui::TreeNode(element_label.c_str()))
						{
							entt::meta_type element_type = element.type();
							DrawMetaObject(entity, component_name, element_type, element);
							ImGui::TreePop();
						}

						ImGui::SameLine();
						if (ImGui::SmallButton("X"))
						{
							view.erase(it);
							ImGui::PopID();
							break;
						}

						ImGui::PopID();
						++index;
					}

					if (ImGui::Button(" + Add Element "))
					{
						entt::meta_type inner_type = view.value_type();

						if (inner_type) {
							entt::meta_any new_element = inner_type.construct();
							if (new_element) {
								view.insert(view.end(), new_element);
							}
						}
					}

					ImGui::TreePop();
				}
			}

			else if (field.type().is_class()) {
				// Nested struct: recurse
				if (ImGui::TreeNode(label.c_str())) {
					auto f = field.type();
					DrawMetaObject(entity, component_name, f, field);
					ImGui::TreePop();
				}
			}
			else {
				ImGui::TextDisabled("%s (unhandled type)", label.c_str());
			}

			ImGui::PopID();
		}
	}
}

#endif
