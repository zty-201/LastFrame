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
#include <algorithm>
#include <cctype>
#include <cstdlib>

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
#include "UISystem.hpp"
// To rename objects
#include "NameComponent.h"
#include "MenuManager.hpp"
#include "UIComponent.h"
namespace EngineFramework
{
	namespace
	{
		constexpr const char* kEntityPayloadType = "OBJECT_MANAGER_ENTITY_PAYLOAD";
	}

	ObjectManager::ObjectManager() {
		s_Instance = this;
		LOGI("Lfeature: ObjectManager CREATED");
	}
	ObjectManager::~ObjectManager() {
		if (s_Instance == this)
		{
			s_Instance = nullptr;
		}
	}
	void ObjectManager::Initialize() {

	}

	void ObjectManager::RemoveEntityFromFilters(entt::entity entity)
	{
		if (s_Instance)
		{
			s_Instance->RemoveEntityFromAllFilters(entity);
		}
	}

	std::string ObjectManager::TrimString(const std::string& value)
	{
		auto first = std::find_if_not(value.begin(), value.end(), [](unsigned char ch) { return std::isspace(ch); });
		auto last = std::find_if_not(value.rbegin(), value.rend(), [](unsigned char ch) { return std::isspace(ch); }).base();
		if (first >= last)
		{
			return {};
		}
		return std::string(first, last);
	}

	bool ObjectManager::AddEntityToFilter(CustomFilter& filter, entt::entity entity)
	{
		if (entity == entt::null || !registry.valid(entity))
		{
			return false;
		}
		if (std::find(filter.members.begin(), filter.members.end(), entity) != filter.members.end())
		{
			return false;
		}
		filter.members.push_back(entity);
		return true;
	}

	void ObjectManager::RemoveEntityFromFilter(CustomFilter& filter, entt::entity entity)
	{
		filter.members.erase(std::remove(filter.members.begin(), filter.members.end(), entity), filter.members.end());
	}

	bool ObjectManager::IsEntityInFilters(entt::entity entity) const
	{
		if (entity == entt::null)
		{
			return false;
		}
		for (const auto& filter : m_CustomFilters)
		{
			if (std::find(filter.members.begin(), filter.members.end(), entity) != filter.members.end())
			{
				return true;
			}
		}
		return false;
	}

	void ObjectManager::RemoveEntityFromAllFilters(entt::entity entity)
	{
		for (auto& filter : m_CustomFilters)
		{
			RemoveEntityFromFilter(filter, entity);
		}
	}

	void ObjectManager::CleanupFilters()
	{
		for (auto& filter : m_CustomFilters)
		{
			filter.members.erase(
				std::remove_if(filter.members.begin(), filter.members.end(),
					[](entt::entity entity)
					{
						return entity == entt::null || !registry.valid(entity);
					}),
				filter.members.end());
		}
	}

	void ObjectManager::DrawCustomFilters(iEditorView& selection_state)
	{
		ImGui::Separator();
		ImGui::Text("Custom Filters");
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.6f);
		ImGui::InputTextWithHint("##CustomFilterName", "New filter name", m_NewFilterNameBuffer.data(), m_NewFilterNameBuffer.size());
		ImGui::SameLine();
		if (ImGui::SmallButton("Create Filter"))
		{
			std::string name = TrimString(m_NewFilterNameBuffer.data());
			if (!name.empty())
			{
				CustomFilter new_filter{};
				new_filter.name = std::move(name);
				m_CustomFilters.emplace_back(std::move(new_filter));
				m_NewFilterNameBuffer.fill(0);
			}
		}
		ImGui::SameLine();
		if (ImGui::SmallButton("Clear Filters"))
		{
			m_CustomFilters.clear();
		}

		ImGui::Selectable("Drop here to keep entity unfiltered", false, ImGuiSelectableFlags_Disabled);
		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(kEntityPayloadType))
			{
				entt::entity dropped = *static_cast<const entt::entity*>(payload->Data);
				RemoveEntityFromAllFilters(dropped);
			}
			ImGui::EndDragDropTarget();
		}

		if (m_CustomFilters.empty())
		{
			ImGui::TextDisabled("No filters created yet.");
			return;
		}

		for (size_t i = 0; i < m_CustomFilters.size();)
		{
			auto& filter = m_CustomFilters[i];
			ImGui::PushID(static_cast<int>(i));
			ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_OpenOnArrow;
			if (filter.expanded)
			{
				flags |= ImGuiTreeNodeFlags_DefaultOpen;
			}

			bool open = ImGui::TreeNodeEx(filter.name.c_str(), flags);
			bool remove_filter = false;

			if (ImGui::BeginPopupContextItem("FilterContextMenu"))
			{
				if (ImGui::MenuItem("Delete Filter"))
				{
					remove_filter = true;
				}
				ImGui::EndPopup();
			}

			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(kEntityPayloadType))
				{
					entt::entity dropped = *static_cast<const entt::entity*>(payload->Data);
					RemoveEntityFromAllFilters(dropped);
					AddEntityToFilter(filter, dropped);
				}
				ImGui::EndDragDropTarget();
			}

			if (remove_filter)
			{
				if (open)
				{
					ImGui::TreePop();
				}
				ImGui::PopID();
				m_CustomFilters.erase(m_CustomFilters.begin() + static_cast<std::ptrdiff_t>(i));
				continue;
			}

			filter.expanded = open;
			if (open)
			{
				if (filter.members.empty())
				{
					ImGui::TextDisabled("Drag entities here to populate this filter.");
				}

				for (size_t member_index = 0; member_index < filter.members.size();)
				{
					entt::entity member = filter.members[member_index];
					if (member == entt::null || !registry.valid(member))
					{
						filter.members.erase(filter.members.begin() + static_cast<std::ptrdiff_t>(member_index));
						continue;
					}

					ImGui::PushID(static_cast<int>(member_index));
					std::string label = GameObjectSystem::GetEntityName(member);
					bool is_selected = selection_state.IsEntitySelected(member);
					if (ImGui::Selectable(label.c_str(), is_selected))
					{
						selection_state.SetEntitySelection(member);
						selection_state.latest_selected = iEditorView::Latest_Selected::INSTANCE_SELECTED;
					}

					if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
					{
						entt::entity payload = member;
						ImGui::SetDragDropPayload(kEntityPayloadType, &payload, sizeof(payload));
						ImGui::TextUnformatted(label.c_str());
						ImGui::EndDragDropSource();
					}
					ImGui::PopID();
					++member_index;
				}

				ImGui::TreePop();
			}

			ImGui::PopID();
			++i;
		}
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

			// For the searching of object in object manager
			ImGui::Text("Scene Hierarchy");

			// Keep a static buffer to read the user's query
			static char search_buffer[128]{};
			float button_width = ImGui::CalcTextSize("Clear##ObjectSearch").x + ImGui::GetStyle().FramePadding.x * 2.0f;
			float available_width = ImGui::GetContentRegionAvail().x;
			float search_width = std::max(available_width - button_width - ImGui::GetStyle().ItemSpacing.x, 120.0f);
			ImGui::SetNextItemWidth(search_width);
			ImGui::InputTextWithHint("##ObjectSearch", "Search by name", search_buffer, sizeof(search_buffer));
			ImGui::SameLine();
			if (ImGui::SmallButton("Clear##ObjectSearch") && search_buffer[0] != '\0') {
				search_buffer[0] = '\0';
			}
			const std::string search_query_raw = search_buffer;
			const bool has_search_query = std::any_of(search_query_raw.begin(), search_query_raw.end(), [](unsigned char c) { return !std::isspace(c); });

			// Lowercase all the search queries input by the users so that the comparisons are case insensitive to prevent typo errors
			std::string search_query_lower = has_search_query ? search_query_raw : std::string{};
			if (has_search_query) {
				std::transform(search_query_lower.begin(), search_query_lower.end(), search_query_lower.begin(), [](unsigned char c) {
					return static_cast<char>(std::tolower(c));
					});
			}
			const bool numeric_search = has_search_query && std::all_of(search_query_raw.begin(), search_query_raw.end(), [](unsigned char c) {
				return std::isdigit(c);
				});
			uint32_t search_id = 0;
			if (numeric_search) {
				search_id = static_cast<uint32_t>(std::strtoul(search_query_raw.c_str(), nullptr, 10));
			}
			static bool open_popup_once{};
			static entt::entity copied_entity = entt::null;

			// For renaming purposes
			static bool open_rename_popup{};
			static entt::entity rename_target = entt::null;
			static char rename_buffer[128]{};

			iEditorView& selection_state = LevelEditor::GetInstance().variables;
			CleanupFilters();
			DrawCustomFilters(selection_state);
			ImGui::Separator();

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

				if (MenuManager::GetInstance().is_menu_running)
				{					
					MenuManager::GetInstance().add_element(duplicated_entity);
				}

				selection_state.SetEntitySelection(duplicated_entity);
				selection_state.latest_selected = iEditorView::Latest_Selected::INSTANCE_SELECTED;

				GraphicsSystem::GetInstance().GetInstancingSystem()->BuildSceneBatches();

				return duplicated_entity;
			};

			auto delete_entity = [&](entt::entity target_entity) -> bool {
				if (target_entity == entt::null || !registry.valid(target_entity)) {
					return false;
				}

				std::unique_ptr<Record> entity_history{};
				HistorySystem* history_system = HistorySystem::this_class ? &HistorySystem::GetInstance() : nullptr;
				if (history_system) {
					entity_history = history_system->start_track(target_entity, Record_Type::ENTITY_REMOVED);
				}

				registry.destroy(target_entity);
				RemoveEntityFromAllFilters(target_entity);

				if (MenuManager::GetInstance().is_menu_running)
				{
					MenuManager::GetInstance().remove_element(target_entity);
				}

				if (history_system) {
					history_system->end_track(entity_history);
				}

				selection_state.RemoveEntitySelection(target_entity);
				if (!selection_state.HasEntitySelection()) {
					selection_state.latest_selected = iEditorView::Latest_Selected::NONE;
				}

				GraphicsSystem::GetInstance().GetInstancingSystem()->BuildSceneBatches();
				return true;
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

				if (ImGui::Shortcut(ImGuiKey_Delete, copy_paste_flags)) {
					entt::entity active_entity = selection_state.selected_objectID;
					if (!delete_entity(active_entity)) {
						LOGW("[ObjectManager] No valid entity selected to delete");
					}
				}
			}

			// Iterate through here to allow the search of object name
			int incrementing_imgui_id{};
			for (auto entity : entities) {

				if (IsEntityInFilters(entity))
				{
					continue;
				}

				/*
					DISPLAYING OF MENU ENTITIES ONLY WHEN MENU IS OPENED
				*/
				if (MenuManager::GetInstance().is_menu_running)
				{
					const auto& menu_entities = UI_System::GetInstance().getMenu_Entities(MenuManager::GetInstance().GetCurrentMenu());

					// if entity NOT found in menu_entities skip
					if (std::find(menu_entities.begin(),
						menu_entities.end(),
						entity) == menu_entities.end())
					{
						continue;
					}
				}


				std::string label = GameObjectSystem::GetEntityName(entity);
				bool passes_search = true;
				if (has_search_query) {
					const bool id_match = numeric_search && static_cast<uint32_t>(entity) == search_id;
					bool name_match = false;

					// Only entities whose name contains the substring remain visibile, if query is empty then just shows the entire object manager as per usual
					if (!search_query_lower.empty()) {
						std::string label_lower = label;
						std::transform(label_lower.begin(), label_lower.end(), label_lower.begin(), [](unsigned char c) {
							return static_cast<char>(std::tolower(c));
							});
						name_match = label_lower.find(search_query_lower) != std::string::npos;
					}
					passes_search = id_match || name_match;
				}
				if (!passes_search) {
					continue;
				}
				ImGui::PushID(incrementing_imgui_id++);
				CALL_AT_END(imgui_popid, ImGui::PopID(););
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

				if (registry.valid(entity) && ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
				{
					entt::entity payload = entity;
					ImGui::SetDragDropPayload(kEntityPayloadType, &payload, sizeof(payload));
					ImGui::TextUnformatted(label.c_str());
					ImGui::EndDragDropSource();
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

						delete_entity(entity);
						ImGui::EndPopup();
						break; // Exit the loop safely
					}
					// --- Create archtype from selected entity ---

					if (!MenuManager::GetInstance().is_menu_running)
					{
						std::string instance_archtype_label = "Create Archtype From Entity##" + std::to_string((uint32_t)entity);
						if (ImGui::MenuItem(instance_archtype_label.c_str())) {

							instance_to_create_archtype = entity;
							open_popup_once = true;
							ImGui::EndPopup();
							break;
						}
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

			if (MenuManager::GetInstance().is_menu_running)
			{
				UIComponent instance_ui;
				instance_ui.position = { 500.f,500.f }; //just some default value
				instance_ui.size = { 250.f,250.f }; //just some default value
				instance_ui.visible = true; //so its visible when we add, but need to choose texture
				instance_ui.belong_to_menu = true;
				instance_ui.Overlay_name = MenuManager::GetInstance().GetCurrentMenu();		

				registry.emplace<UIComponent>(instance, instance_ui);
				MenuManager::GetInstance().add_element(instance);
				
			}
			else {

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
