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
#include <cstdio>

//Entt reflection system
#include <entt/meta/meta.hpp>
#include <entt/meta/resolve.hpp>
//Entt ECS system
#include <entt/entity/registry.hpp>
#include "System_LevelEditor.hpp"

//Casting meta data to type.
#include "ReflectionRegister.hpp"

#include "ObjectManager.hpp"

//Getting entity archtype name
#include "System_GameObjectSystem.hpp"

//For creation of instance where camera is looking.
#include "GraphicsSystem.h"
#include "CameraComponent.h"
#include "TransformComponent.h"
#include "Mesh.h"
#include "MeshComponent.h"
#include "HistorySystem.hpp"

// To rename objects
#include "NameComponent.h"

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
			static bool open_popup_once{};
			static entt::entity copied_entity = entt::null;

			// For renaming purposes
			static bool open_rename_popup{};
			static entt::entity rename_target = entt::null;
			static char rename_buffer[128]{};

			iEditorView& selection_state = LevelEditor::GetInstance().variables;


			auto duplicate_and_select = [&](entt::entity source_entity) -> entt::entity {
				if (source_entity == entt::null || !registry.valid(source_entity)) {
					LOGW("[ObjectManager] Cannot duplicate invalid entity");
					return entt::null;
				}

				entt::entity duplicated_entity = GameObjectSystem::GetInstance().DuplicateInstance(source_entity);
				if (duplicated_entity == entt::null) {
					return entt::null;
				}

				if (HistorySystem::this_class) {
					HistorySystem& history_system = HistorySystem::GetInstance();
					auto record = history_system.start_track(duplicated_entity, Record_Type::ENTITY_ADDED);
					history_system.end_track(record);
				}

				selection_state.SetEntitySelection(duplicated_entity);
				selection_state.latest_selected = iEditorView::Latest_Selected::INSTANCE_SELECTED;
				return duplicated_entity;
			};

			const bool window_focused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
			const bool window_hovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);

			// Ensure that the objectmanager window is being focused on
			const bool allow_copy_paste = (window_focused || window_hovered) && !ImGui::GetIO().WantTextInput;
			const ImGuiInputFlags copy_paste_flags = ImGuiInputFlags_RouteGlobal | ImGuiInputFlags_RouteOverFocused;

			// If copy paste is possible (when window is being focus on)
			// Ctrl + C caches the currently selected entity in copied_entity
			if (allow_copy_paste) {
				if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_C, copy_paste_flags)) {
					entt::entity active_entity = selection_state.selected_objectID;
					if (active_entity != entt::null && registry.valid(active_entity)) {
						copied_entity = active_entity;
					}
					else {
						LOGW("[ObjectManager] No valid entity selected to copy"); 
					}
				}

				// Ctrl + V reuses duplicate_and_select function to get the copied entity and paste the instance which is also tracked by history/selection
				// Hnece duplicated instances can be redo/undo etc
				if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_V, copy_paste_flags)) {
					if (copied_entity == entt::null) {
						LOGW("[ObjectManager] No copied entity available to paste. Use Ctrl+C first.");
					}
					else if (!registry.valid(copied_entity)) {
						LOGW("[ObjectManager] Copied entity is no longer valid. Copy again.");
						copied_entity = entt::null;
					}
					else {
						duplicate_and_select(copied_entity);
					}
				}

				// Pressing F2 opens the rename popup for the currently selected entity
				if (ImGui::Shortcut(ImGuiKey_F2, copy_paste_flags)) {
					entt::entity active_entity = selection_state.selected_objectID;
					if (active_entity != entt::null && registry.valid(active_entity)) {
						rename_target = active_entity;
						std::string current_name = GameObjectSystem::GetEntityName(active_entity);
						std::snprintf(rename_buffer, sizeof(rename_buffer), "%s", current_name.c_str());
						rename_buffer[sizeof(rename_buffer) - 1] = '\0';
						open_rename_popup = true;
					}
					else {
						LOGW("[ObjectManager] No valid entity selected to rename");
					}
				}
			}

			int incrementing_imgui_id{};
			for (auto entity : entities) {
				ImGui::PushID(incrementing_imgui_id++);
				CALL_AT_END(imgui_popid, ImGui::PopID(););
				std::string label = GameObjectSystem::GetEntityName(entity);
				//ImGui::Text("%s", label.c_str());

				// Determine if this entity is selected
				bool is_selected = selection_state.IsEntitySelected(entity);

				// Make the entity a selectable item
				if (ImGui::Selectable(label.c_str(), is_selected)) {
					ImGuiIO& io = ImGui::GetIO();
					bool ctrl_down = io.KeyCtrl;
					bool shift_down = io.KeyShift;

					if (ctrl_down) {
						if (is_selected) {
							selection_state.RemoveEntitySelection(entity);
							if (!selection_state.HasEntitySelection()) {
								selection_state.latest_selected = iEditorView::Latest_Selected::NONE;
							}
						}
					}
					else {
						if (shift_down) {
							selection_state.AddEntitySelection(entity);
						}
						else {
							selection_state.SetEntitySelection(entity);
						}

						selection_state.latest_selected = iEditorView::Latest_Selected::INSTANCE_SELECTED;
						LOGI(("Selected Entity: " + std::to_string((uint32_t)entity)).c_str());

						if (registry.all_of<MeshComponent>(entity)) {
							const auto& mc = registry.get<MeshComponent>(entity);

							// Build object-space AABB from MeshComponent authoring values
							Mesh::AABB obj{};
							obj.minV = glm::vec3(mc.localMin[0], mc.localMin[1], mc.localMin[2]);
							obj.maxV = glm::vec3(mc.localMax[0], mc.localMax[1], mc.localMax[2]);

							// World-space if we also have a transform
							if (registry.all_of<TransformComponent>(entity)) {
								const auto& tf = registry.get<TransformComponent>(entity);
								const Mesh::AABB world = TransformAABB(obj, tf.modelMatrix);

								LOGI("[AABB] ent=%u  "
									"objMin(%.3f,%.3f,%.3f) objMax(%.3f,%.3f,%.3f)  "
									"worldMin(%.3f,%.3f,%.3f) worldMax(%.3f,%.3f,%.3f)",
									(uint32_t)entity,
									obj.minV.x, obj.minV.y, obj.minV.z,
									obj.maxV.x, obj.maxV.y, obj.maxV.z,
									world.minV.x, world.minV.y, world.minV.z,
									world.maxV.x, world.maxV.y, world.maxV.z);
							}
							else {
								LOGI("[AABB] ent=%u  objMin(%.3f,%.3f,%.3f) objMax(%.3f,%.3f,%.3f)  (no TransformComponent)",
									(uint32_t)entity,
									obj.minV.x, obj.minV.y, obj.minV.z,
									obj.maxV.x, obj.maxV.y, obj.maxV.z);
							}
						}
						else {
							LOGI("[AABB] ent=%u  (no MeshComponent)", (uint32_t)entity);
						}
					}
				}

				// Check for right-click while the item is hovered/active
				if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
					// Open the popup menu attached to this item
					ImGui::OpenPopup("EntityContextMenu");
				}

				std::string popupID = "EntityContextMenu##" + std::to_string((uint32_t)entity);

				// Check for right-click on the selectable item AND start the menu
				if (ImGui::BeginPopupContextItem(popupID.c_str())) {

					// Set the context to the current selected entity
					selection_state.SetEntitySelection(entity);
					selection_state.latest_selected = iEditorView::Latest_Selected::INSTANCE_SELECTED;

					// Add the actions to be tracked by history system to ensure the redo/undo works for duplicating and delete etc
					std::string duplicateLabel = "Duplicate Entity##" + std::to_string((uint32_t)entity);
					if (ImGui::MenuItem(duplicateLabel.c_str())) {
						duplicate_and_select(entity);
						ImGui::EndPopup();
						break;
					}

					std::string deleteLabel = "Delete Entity##" + std::to_string((uint32_t)entity);

					if (ImGui::MenuItem(deleteLabel.c_str())) {

						std::unique_ptr<EngineFramework::Record> entity_history{};
						HistorySystem* history_system = HistorySystem::this_class ? &HistorySystem::GetInstance() : nullptr;
						if (history_system) {
							entity_history = history_system->start_track(entity, Record_Type::ENTITY_REMOVED);
						}

						// 1. Destroy the entity
						registry.destroy(entity);

						if (history_system) {
							history_system->end_track(entity_history);
						}

						// 2. Clear selection
						selection_state.RemoveEntitySelection(entity);
						if (!selection_state.HasEntitySelection()) {
							selection_state.latest_selected = iEditorView::Latest_Selected::NONE;
						}

						ImGui::EndPopup();
						break; // Exit the loop safely
					}
					// --- Create archtype from selected entity ---
					std::string instance_archtype_label = "Create Archtype From Entity##" + std::to_string((uint32_t)entity);
					if (ImGui::MenuItem(instance_archtype_label.c_str())) {

						instance_to_create_archtype = entity;
						open_popup_once = true;
						ImGui::EndPopup();
						break;
					}

					// To rename the entity
					// Will open a modal popup for user to edit the name
					std::string rename_label = "Rename##" + std::to_string((uint32_t)entity);
					if (ImGui::MenuItem(rename_label.c_str())) {
						rename_target = entity;
						std::string current_name = GameObjectSystem::GetEntityName(entity);
						std::snprintf(rename_buffer, sizeof(rename_buffer), "%s", current_name.c_str());
						rename_buffer[sizeof(rename_buffer) - 1] = '\0';
						open_rename_popup = true;
						ImGui::EndPopup();
						break;
					}

					ImGui::EndPopup();
				}

				// Optional: hover highlight (ImGui automatically applies a default hover style)
				if (ImGui::IsItemHovered()) {
					ImGui::SetTooltip("Entity ID: %u", (uint32_t)entity);
				}

			}

			if (rename_target != entt::null && open_rename_popup)
			{
				ImGui::OpenPopup("Rename Entity");
				open_rename_popup = false;
			}

			// In the modal popup, user can edit the name
			// If the input is empty, remove the NameComponent
			// Update the existing component if NameComponent is present
			// Emplace a fresh NameComponent if missing
			if (ImGui::BeginPopupModal("Rename Entity"))
			{
				CALL_AT_END(imgui_endrename, ImGui::EndPopup(););
				auto reset_rename_state = [&]()
				{
					rename_target = entt::null;
					rename_buffer[0] = '\0';
				};

				if (rename_target == entt::null || !registry.valid(rename_target))
				{
					reset_rename_state();
					ImGui::CloseCurrentPopup();
				}
				else
				{
					ImGui::InputText("Name", rename_buffer, sizeof(rename_buffer));

					auto finalize_rename = [&](const char* new_name)
					{
						HistorySystem* history_system = HistorySystem::this_class ? &HistorySystem::GetInstance() : nullptr;
						bool has_component = registry.all_of<NameComponent>(rename_target);
						const std::string desired_name = new_name ? std::string(new_name) : std::string{};

						if (desired_name.empty())
						{
							if (has_component)
							{
								std::unique_ptr<Record> record = history_system ? history_system->start_track(rename_target, "NameComponent", Record_Type::COMP_REMOVED) : nullptr;
								registry.remove<NameComponent>(rename_target);
								if (history_system)
								{
									history_system->end_track(record);
								}
							}
							return;
						}

						if (has_component)
						{
							std::unique_ptr<Record> record = history_system ? history_system->start_track(rename_target, "NameComponent", "name") : nullptr;
							registry.get<NameComponent>(rename_target).name = desired_name;
							if (history_system)
							{
								history_system->end_track(record, entt::meta_any{ desired_name });
							}
						}
						else
						{
							registry.emplace<NameComponent>(rename_target, NameComponent{ desired_name });
							if (history_system)
							{
								std::unique_ptr<Record> record = history_system->start_track(rename_target, "NameComponent", Record_Type::COMP_ADDED);
								history_system->end_track(record);
							}
						}
					};

					if (ImGui::Button("Confirm"))
					{
						finalize_rename(rename_buffer);
						reset_rename_state();
						ImGui::CloseCurrentPopup();
					}
					ImGui::SameLine();
					if (ImGui::Button("Cancel"))
					{
						reset_rename_state();
						ImGui::CloseCurrentPopup();
					}
				}
			}

			if (instance_to_create_archtype != entt::null && open_popup_once)
			{
				ImGui::OpenPopup("Create Archtype From Instance");
				open_popup_once = false;
			}
			
			//Create archtype from instance, and assign archtype to that instance.
			if (ImGui::BeginPopupModal("Create Archtype From Instance"))
			{
				CALL_AT_END(imgui_endpopup, ImGui::EndPopup(););
				constexpr int buffer_size{ 128 };
				static char name_buffer[buffer_size] = "";
				static char model_type_buffer[buffer_size] = "";
				ImGui::InputText("Archtype name", name_buffer, buffer_size);
				ImGui::InputText("Archtype model_type", model_type_buffer, buffer_size);
				if (ImGui::Button("Confirm"))
				{
					CreateArchtypeFromInstance(instance_to_create_archtype, name_buffer, model_type_buffer);
					//Clear buffers.
					instance_to_create_archtype = entt::null;
					name_buffer[0] = '\0';
					model_type_buffer[0] = '\0';
					ImGui::CloseCurrentPopup();
				}
				if (ImGui::Button("Close"))
				{
					//Ignore event, user has chosen not to give enough information.
					//Clear buffers.
					instance_to_create_archtype = entt::null;
					name_buffer[0] = '\0';
					model_type_buffer[0] = '\0';
					ImGui::CloseCurrentPopup();
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
		/*if (ImGui::Button("Create Empty Object"))
		{
			entt::entity instance = registry.create();
			UNREFERENCED_PARAMETER(instance);
		}*/
		if (ImGui::Button("Create Empty Object"))
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


	void ObjectManager::CreateArchtypeFromInstance(entt::entity entity, const std::string& name, const std::string& model_type)
	{
		/*
			Validity checks
			- Whether said archtype already exists
			- Whether entity is not valid.
		*/
		if (!registry.valid(entity))
		{
			LOGE("[ObjectManager::CreateArchtypeFromInstance] Entity %lu is not a valid entity!", (unsigned long)entity);
			return; //Don't do anything.
		}
		if (name.empty())
		{
			LOGE("%s", "[ObjectManager::CreateArchtypeFromInstance] Cannot create archtype with no name");
			return; //Don't do anything.
		}
		if (AssetManagerSystem::HasResource(AssetManagerSystem::GetFullGuid(AssetManagerSystem::ARCHTYPE_GUID, name)))
		{
			LOGE("[ObjectManager::CreateArchtypeFromInstance] Archtype %s already exists!", name.c_str());
			return; //Don't do anything.
		}


		/*
			Get the archtype from instance
		*/
		Archtype created_archtype = GameObjectSystem::CreateArchtypeFromInstance(entity, model_type);

		/*
			Create an archtype file from the archtype, then reload to create an archtype asset with guid.
		*/
		std::filesystem::path archtype_folder = AssetManagerSystem::GetAssetFolderPrefix() / AssetManagerSystem::GetFolderPath(AssetManagerSystem::ARCHTYPE);
		std::string archtype_extension = name + ".json";
		std::filesystem::path archtype_path = archtype_folder / archtype_extension;

		//Ensure the file doesn't already exist.
		if (std::filesystem::is_regular_file(archtype_path))
		{
			LOGE("[ObjectManager::CreateArchtypeFromInstance] Archtype json file for %s already exists!", name.c_str());
			return;
		}

		//Serialize the archtype into the created file, thereby copying it.
		std::ofstream archtype_filestream{ archtype_path };
		archtype_filestream << created_archtype.Serialize() << std::flush;
		archtype_filestream.close();
		//Update archtypes with this new change.
		AssetManagerSystem::ReloadAllResourcesOfType(AssetManagerSystem::ARCHTYPE, AssetManagerSystem::ARCHTYPE_GUID);

		/*
			Assign the archtype to the entity that created it.
		*/
		xresource::full_guid archtype_guid = AssetManagerSystem::GetFullGuid(AssetManagerSystem::ARCHTYPE_GUID, name);
		if (archtype_guid == xresource::full_guid{})
		{
			LOGE("[ObjectManager::CreateArchtypeFromInstance] Unable to get guid for Archtype %s", name.c_str());
			return;
		}
		GameObjectSystem::AssignArchtypeToInstance(entity, archtype_guid.m_Instance);
	}
}
#endif
