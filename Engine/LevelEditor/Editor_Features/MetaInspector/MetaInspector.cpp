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

#include "pch.hpp"
#include "MetaInspector.hpp"
#include "CameraComponent.h" // Testing purposes of property manager
#include "MeshComponent.h"
#include "LightComponent.h"
#include "TransformComponent.h"
#include "MaterialComponent.h"
#include "TextureComponent.h"
#include "AssetManager.hpp"
#include "scene.h"
#include <string_view>
#include <imgui.h>
#include <array>
#include <iomanip>
#include <sstream>

// Temporary shim: some compilers require the tag types backing xresource::guid to be complete.
namespace xresource {
	struct rsc_instance_guid_tag {};
	struct rsc_instance_guid_large_tag {};
	struct rsc_type_guid_tag {};
} // namespace xresource

// FILE TO HANDLE THE META DATA OF THE ENTITIY
namespace EditorFeatures {

	static const std::unordered_map<std::string_view, std::unordered_set<std::string_view>> hidden_fields = {
		{"CameraComponent", {"view_vector", "proj_matrix", "oldx", "oldy"}},
		{"MeshComponent", {"vao", "indexCount"}},
		{"LightComponent", {"L"}},
		{"TransformComponent", {"modelMatrix"}},
		{"MaterialComponent", {"Ka", "Kd", "Ks", "shininess", "effect"}}
	};

	static std::string to_lower(std::string s) {
		std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
		return s;
	}

	//static constexpr entt::id_type DISPLAY_NAME_ID = entt::hashed_string::value("display.name");
	//static constexpr entt::id_type UI_COLOR_ID = entt::hashed_string::value("ui.color");

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

	static std::string describe_guid_name(const xresource::full_guid& guid, std::string display)
	{
		if (guid.m_Type == EngineFramework::AssetManagerSystem::TEXTURE_GUID) {
			return display;
		}
		if (guid.m_Type == EngineFramework::AssetManagerSystem::CUBEMAP_GUID) {
			return display + " (Cubemap)";
		}
		return display + " (Unknown)";
	}

	static bool draw_texture_guid_picker(const char* label, xresource::full_guid& guid, const char* none_label = "None")
	{
		using EngineFramework::AssetManagerSystem;
		using SceneTexture = Scene::Texture;

		const auto textures = AssetManagerSystem::GetAllResourceOfType<SceneTexture>(AssetManagerSystem::TEXTURE_GUID);
		const auto cubemaps = AssetManagerSystem::GetAllResourceOfType<SceneTexture>(AssetManagerSystem::CUBEMAP_GUID);

		std::string current = guid.empty() ? none_label : AssetManagerSystem::GetName(guid.m_Instance);
		if (current.empty()) {
			current = format_guid_hex(guid.m_Instance.m_Value);
		}
		if (!guid.empty()) {
			current = describe_guid_name(guid, current);
		}

		bool changed = false;
		if (ImGui::BeginCombo(label, current.c_str())) {
			if (ImGui::Selectable(none_label, guid.empty())) {
				guid.clear();
				changed = true;
			}

			auto draw_entries = [&](const auto& list, const xresource::type_guid& type, std::string_view suffix) {
				for (const auto& [instance, _] : list) {
					std::string name = AssetManagerSystem::GetName(instance);
					if (name.empty()) {
						name = format_guid_hex(instance.m_Value);
					}
					const xresource::full_guid candidate{ instance, type };
					std::string label_text = suffix.empty() ? name : (name + " " + std::string{ suffix });
					const bool selected = (!guid.empty() && guid == candidate);
					if (ImGui::Selectable(label_text.c_str(), selected)) {
						guid = candidate;
						changed = true;
					}
					if (selected) {
						ImGui::SetItemDefaultFocus();
					}
				}
			};

			draw_entries(textures, AssetManagerSystem::TEXTURE_GUID, {});
			draw_entries(cubemaps, AssetManagerSystem::CUBEMAP_GUID, "(Cubemap)");
			ImGui::EndCombo();
		}
		return changed;
	}

	void DrawMetaObject(entt::meta_type& mt, entt::meta_any& obj) {
		//const char* typeName = mt.name();

		auto t = obj.type();
		
		// Each entry is pair<entt::id_type, entt::meta_data> - for self reference
		for (auto&& pair : mt.data()) {
			const entt::id_type id = pair.first;
			const entt::meta_data data = pair.second;

			const std::string label = label_for(data, id);
			const char* type_name = mt.name();
			const std::string_view type_key = (type_name && *type_name) ? std::string_view{ type_name } : std::string_view{};

			if (const auto hidden = hidden_fields.find(type_key);
				hidden != hidden_fields.end() && hidden->second.count(label)) {
				continue;
			}

			entt::meta_any field = data.get(obj);

			// Skip the hidden fields

			if (field.type() == entt::resolve<int>()) {
				int& v = field.cast<int&>();
				if (ImGui::DragInt(label.c_str(), &v, 1.0f)) {
					data.set(obj, v);
				}
			}
			else if (field.type().is_enum()) {
				entt::meta_any tmp = field;                  // copy keeps original untouched
				if (!tmp.allow_cast<int>()) {
					ImGui::TextDisabled("%s (enum, no int conversion)", label.c_str());
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
						value = static_cast<CameraComponent::CameraType>(current);
						data.set(obj, value);
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
					value = static_cast<CameraComponent::CameraType>(current);
					data.set(obj, value);
				}
			}

			else if (field.type() == entt::resolve<float>()) {
				float& v = field.cast<float&>();
				if (ImGui::DragFloat(label.c_str(), &v, 0.01f)) {
					data.set(obj, v);
				}
			}

			else if (field.type() == entt::resolve<double>()) {
				double& v = field.cast<double&>();
				if (ImGui::InputDouble(label.c_str(), &v)) {
					data.set(obj, v);
				}
			}

			else if (field.type() == entt::resolve<bool>()) {
				bool& v = field.cast<bool&>();
				if (ImGui::Checkbox(label.c_str(), &v)) {
					data.set(obj, v);
				}
			}

			else if (field.type() == entt::resolve<std::string>()) {
				std::string& s = field.cast<std::string&>();
				char buffer[512];
				std::snprintf(buffer, sizeof(buffer), "%s", s.c_str());
				if (ImGui::InputText(label.c_str(), buffer, sizeof(buffer))) {
					data.set(obj, std::string{ buffer });
				}
			}

			else if (field.type() == entt::resolve<glm::vec3>()) {
				glm::vec3& v = field.cast<glm::vec3&>();

				// Heuristic color picker
				std::string lower_name = to_lower(label);
				bool colorish = (lower_name.find("color") != std::string::npos) || (lower_name.find("albedo") != std::string::npos) || (lower_name.find("tint") != std::string::npos);

				if (colorish) {
					float color[3]{ v.x, v.y, v.z };
					if (ImGui::ColorEdit3(label.c_str(), color)) {
						v = { color[0], color[1], color[2] };
						data.set(obj, v);
					}
				}
				else {
					draw_vec3_drag(label.c_str(), v);
					data.set(obj, v);
				}
			}
			else if (field.type() == entt::resolve<glm::vec4>()) {
				glm::vec4& v = field.cast<glm::vec4&>();

				std::string lower_name = to_lower(label);
				bool colorish = (lower_name.find("color") != std::string::npos) || (lower_name.find("albedo") != std::string::npos) || (lower_name.find("tint") != std::string::npos);

				if (colorish) {
					float color[4]{ v.x, v.y, v.z, v.w };
					if (ImGui::ColorEdit4(label.c_str(), color)) {
						v = { color[0], color[1], color[2], color[3] };
						data.set(obj, v);
					}
				}
				else {
					draw_vec4_drag(label.c_str(), v);
					data.set(obj, v);
				}
			}

			else if (field.type() == entt::resolve<xresource::full_guid>()) {
				auto& guid = field.cast<xresource::full_guid&>();
				const bool is_texture = type_key == "TextureComponent";
				const bool is_normal = type_key == "NormalMapComponent";
				if (is_texture || is_normal) {
					if (draw_texture_guid_picker(label.c_str(), guid)) {
						data.set(obj, guid);
					}
				}
				else {
					ImGui::TextDisabled("%s (full_guid editor not implemented)", label.c_str());
				}
			}

			else if (field.type().is_class()) {
				// Nested struct: recurse
				if (ImGui::TreeNode(label.c_str())) {
					auto f = field.type();
					DrawMetaObject(f, field);
					ImGui::TreePop();
				}
			}
			else {
				ImGui::TextDisabled("%s (unhandled type)", label.c_str());
			}
		}
	}
}
