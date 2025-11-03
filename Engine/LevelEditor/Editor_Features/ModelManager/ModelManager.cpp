/*
*****************************************************************/
/*!
\file   ModelManager.cpp
\author Tan Yee Tong, Joel Lee Jie
\par    yeetong.t\@digipen.edu, joeljie.lee\@digipen.edu
\date   October 2025
\brief
This file shows the models currently loaded in the scene of the level editor
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
#include "entt/core/hashed_string.hpp"

//Models (Archtypes) and Gameobject System
#include "Archtype.hpp"
#include "System_GameObjectSystem.hpp"
#include "ModelManager.hpp"
#include "imgui.h"

//For display of model image
#include "TextureComponent.h"
#include "scene.h"
//#include <iostream>
namespace EngineFramework
{

	/*
	   * TODO:
		   Popup Menu (Right click archtype button):
		   - Enable dragdrop into scene (create instance with default transform at where camera is pointing, with option to choose normalized or world pos).
		   --> First create instance
		   --> Next do registry.emplace_or_replace.
		   --> Do this in a function overload for GameObjectSystem::CreateInstance().
		   - Creation of instance
		   --> Instead of drag and drop, just create as is without transform component added.

		   - Switch archtype model_type

		   - Copy of existing archtype
		   --> Add archtype file with different name.
		   --> Serialize existing archtype, and write to that file.
		   --> Reload archtypes
		   - Rename of archtype
		   --> Rename archtype file and in corresponding guid.meta file.
		   --> Call ModifyNameMappings for assetmanager.
		   - Deletion of archtype
		   --> Delete the archtype file and in corresponding guid.meta file.
		   --> Reload archtypes (so entities won't be saved with said archtype anymore). This also modifies the name_guid and guid_name maps.

		   Others
		   - Creation of new archtype
		   --> Add archtype file
		   --> Reload archtypes

		   When renaming or deleting an archtype, ensure
		   - guid.meta file is changed
		   - ModifyNameMappings or ReloadAllResourcesOfType is called, to update resource maps.
	   */

	ModelManager::ModelManager() {
		LOGI("Lfeature: Model Manager CREATED");
	}
	ModelManager::~ModelManager() {

	}
	void ModelManager::Initialize() {
		selected_types.clear();
	}

	/*
		Draws what types have been selected, as a single string at the top.
	*/
	void ModelManager::DrawSelectedTypes()
	{
		//==Display selected type names at the top
		std::ostringstream selected_types_string{};
		selected_types_string << "Model Types: |";
		for (auto& pair : selected_types)
		{
			//Only write the selected (true) types.
			if (!pair.second) continue;
			selected_types_string << pair.first << "|";
		}
		ImGui::TextWrapped(selected_types_string.str().c_str());
	}

	/*
		Draws the list of model types in the left pane as selectables
		\param spacing per_row
		- The space between elements, number of elements per row of the table.
	*/
	void ModelManager::DrawTypeList(const std::unordered_map<std::string, std::vector<int>>& archtype_type_map)
	{
		//==Display type selection list in the left pane
		const float avail = ImGui::GetContentRegionAvail().x;
		CALL_AT_END(imgui_child_left_pane, ImGui::EndChild(););
		//Based on current window size, make the left pane 1/3 width by default.
		ImGui::BeginChild("Child_LeftPane", ImVec2(avail / 3.f, 0), true, ImGuiChildFlags_ResizeX);
		//Display selectable list, based on model types.
		for (auto it = selected_types.begin(); it != selected_types.end();)
		{
			//Ensure it's a valid type by comparing against existing model types.
			if (!archtype_type_map.contains(it->first))
			{
				//No such archtype belongs to this model type, so delete the type.
				it = selected_types.erase(it);
				continue;
			}
			std::string label = (it->first.empty()) ? "##Unlabelled" : it->first;
			//Toggle whether type is to be displayed or not.
			if (ImGui::Selectable(label.c_str()))
			{
				it->second = !it->second;
			}
			it++;
		}
	}

	//Handles the popup menu when rightclicking the archtype element
	void ModelManager::ArchtypePopupMenu(std::string archtype_name, std::shared_ptr<Archtype> archtype)
	{
		//Open a popup menu if right clicking the element.
		//Determine what archtype event occurs.
		if (!ImGui::BeginPopupContextItem((std::string("Archtype Menu##") + archtype_name).c_str(), ImGuiMouseButton_Right)) return;
		CALL_AT_END(imgui_endpopup, ImGui::EndPopup(););

		//If chosen, only need to give the minimum amt of information necessary for the event to be executed.
		if (ImGui::MenuItem("Copy Archtype"))
		{
			delayed_archtype_event = ArchtypeEvent(ArchtypeEvent::COPY, {}, {}, archtype);
		}
		if (ImGui::MenuItem("Rename Archtype"))
		{
			delayed_archtype_event = ArchtypeEvent(ArchtypeEvent::RENAME, archtype_name);
		}
		if (ImGui::MenuItem("Change ModelType"))
		{
			delayed_archtype_event = ArchtypeEvent(ArchtypeEvent::CHANGE_MODEL_TYPE, {}, {}, archtype);
		}
		if (ImGui::MenuItem("Delete ModelType"))
		{
			delayed_archtype_event = ArchtypeEvent(ArchtypeEvent::DELETE, archtype_name);
		}

	}

	namespace
	{
		//Helper function that draws an archtype image button. 
		GLuint DrawArchtypeElement(const std::string& archtype_name, const std::shared_ptr<Archtype> archtype, float cell_w, entt::id_type texture_comp_id)
		{
			std::vector<entt::meta_any> component_arr = archtype->component_arr;
			GLuint texture_id{};
			/*
				Iterate over all component types and get the texture id if possible.
				If there's an error, avoid displaying texture but still display the model.
			*/
			try
			{
				for (const entt::meta_any& comp : component_arr)
				{
					if (comp.type().id() != texture_comp_id) continue;

					//Get the texture asset for drawing
					xresource::full_guid texture_guid = comp.cast<TextureComponent>().texture_guid;
					std::shared_ptr<Scene::Texture> texture = AssetManagerSystem::GetResource<Scene::Texture>(texture_guid);
					if (texture) texture_id = texture->id;
					break;
				}
			}
			catch (std::exception& e)
			{
				LOGE("Error in Level Editor ModelManager when getting texture: Archtype %s : %s", archtype_name.c_str(), e.what());
			}

			//==Drawing of archtype button. Will not draw an image if not available.
			if (ImGui::ImageButton(archtype_name.c_str(), (ImTextureID)texture_id, ImVec2(cell_w, cell_w), { 0, 1 }, { 1, 0 }))
			{

			}
			return texture_id;
		}
	}


	/*
		Draws the table of archtypes in the right pane.
		\param spacing per_row
		- The space between elements, number of elements per row of the table.
	*/
	void ModelManager::DrawSelectedArchtypeTable(int spacing, int per_row, std::unordered_map<std::string, std::vector<int>>& archtype_type_map, const std::vector<std::pair<xresource::full_guid, std::shared_ptr<Archtype>>>& archtype_arr)
	{

		//==ImGui formatting
		const float avail = ImGui::GetContentRegionAvail().x;
		//Based on current window size, make the right pane 2/3 width by default.
		CALL_AT_END(imgui_child_right_pane, ImGui::EndChild(););
		ImGui::BeginChild("Child_RightPane", ImVec2(2 * avail / 3.f, 0), true, ImGuiChildFlags_ResizeX);
		//==Create child button, on top of the table.
		//==Table Menu, appears when right clicking on the table (instead of archtype element).
		//Open a popup menu if right clicking the table.
		if (ImGui::Button("Create Archtype"))
		{
			delayed_archtype_event = ArchtypeEvent(ArchtypeEvent::CREATE);
		}

		//==Table formatting
		const float table_width = ImGui::GetContentRegionAvail().x;
		const float spacingF = (float)spacing;
		const float cell_w = floorf((table_width - spacingF * (per_row + 1)) / per_row);

		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(spacingF, 8.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(spacingF * 0.5f, 6.0f)); // nice outer/inner padding
		CALL_AT_END(imgui_popstyle, ImGui::PopStyleVar(2););

		/*
		Display all archtypes in table format.
		Archtypes are in separate groups based on the model_type, like "Character" or "Background Object"
		For each group (that is selected) the model_type is printed as the header.
		Afterwards, all archtypes within that group is drawn as buttons into the table cells.
		If the archtype has a texture, that texture is drawn as part of the image button.

		Display
		- Model type header
		- Name of archtype
		- Texture image if possible
		*/

		entt::id_type texture_comp_id = entt::resolve<TextureComponent>().id();
		if (!ImGui::BeginTable("model-grid", per_row, ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_PadOuterX | ImGuiTableFlags_NoBordersInBody)) return;
		CALL_AT_END(imgui_endtable, ImGui::EndTable(););

		//Iterate over all archtypes belonging to the selected model types.
		for (auto& selected_type : selected_types)
		{
			//Only display selected types.
			if (!selected_type.second) continue;
			std::string model_type = selected_type.first;
			std::string model_type_label = "[" + model_type + "]";
			//==At the top of each model type group, print out the model type name
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::TableHeader(model_type_label.c_str());

			//==Display archtypes belonging to that group.
			const std::vector<int>& selected_archtype_arr = archtype_type_map[model_type];
			if (selected_archtype_arr.empty()) continue;
			for (int i = 0; i < selected_archtype_arr.size(); i++)
			{
				int archtype_idx = selected_archtype_arr[i];
				//==Setting of table element formatting
				if (i % per_row == 0) ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(i % per_row);

				//==Accessing archtype name and data (if available).
				auto& pair = archtype_arr[archtype_idx];
				xresource::full_guid archtype_guid = pair.first;
				std::shared_ptr<Archtype> archtype = pair.second;
				std::string archtype_name = AssetManagerSystem::GetName(archtype_guid);
				//Do not push indexes for IDs, as they are unreliable (asset_map is unordered and order of archtypes may change at any time).
				ImGui::PushID(archtype_name.c_str());
				CALL_AT_END(imgui_popid, ImGui::PopID(););

				ImGui::BeginGroup();

				//==Draw Archtype element (image button)
				GLuint archtype_texture = DrawArchtypeElement(archtype_name, archtype, cell_w, texture_comp_id);

				//==Set drag and drop payload for the archtype, to allow scene to receive selected archtype name for instantiation.
				if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
				{
					selected_drag_drop_archtype = archtype_name;
					ImGui::SetDragDropPayload("SELECTED_MODEL_NAME", &selected_drag_drop_archtype, selected_drag_drop_archtype.size());

					ImGui::TextUnformatted(selected_drag_drop_archtype.c_str());
					ImGui::Image((ImTextureID)archtype_texture, ImVec2(50, 50), ImVec2(0, 1), ImVec2(1, 0));

					ImGui::EndDragDropSource();
				}
				//Label the archtype element.
				ImGui::TextWrapped(archtype_name.c_str());

				ImGui::EndGroup();
				//==Archtype popup menu, enabled when right clicking on the archtype element.
				ArchtypePopupMenu(archtype_name, archtype);
			}
		}
	}
	//Display and Management of the Model Manager. 
	//Currently inefficient as all archtype ptrs are copied to map every frame, only optimize if necessary.
	void ModelManager::Run() {

		/*
			Model Manager Display Format:
			- Top: Selected model type names
			- Left pane: Model type selection list
			- Right pane: Archtype table (belonging to the selected types).

			3 containers are used
			- Temporary archtype_arr: Taken from assetmanagersystem, used to get all archtypes.
			- Temporary archtype_type_map: Separates archtypes into their respective model_type.
			- Persistent selected_types (map): Determines which model types the user has toggled for display.

			Temporary containers are refreshed every frame.
			Persistent selected_types will be cleared when the feature is closed.

			Note: Do not persistently store archtype shared ptrs, use GetAllResources instead.
		*/

		//// Give size once; don't lock it every frame
		ImGui::SetNextWindowSize(ImVec2(420, 260), ImGuiCond_FirstUseEver);
		CALL_AT_END(imgui_end, ImGui::End(););
		if (ImGui::Begin("Model Manager", &is_enabled)) {
			//==Table formatting
			static int per_row = 3;
			ImGui::InputInt("Show models per row: ", &per_row, 1, 1);

			static int spacing = 20;
			ImGui::InputInt("Spacing: ", &spacing, 2, 10);

			//lock at min 3 and max 15
			per_row = std::clamp(per_row, 3, 15);
			spacing = std::clamp(spacing, 10, 50);

			//==Array and Map of archtypes.
			//Note! This is inefficient due to copying every frame, but is the simplest way.
			//To make a persistent map ensure that it is updated on every change.
			const std::vector<std::pair<xresource::full_guid, std::shared_ptr<Archtype>>> archtype_arr = AssetManagerSystem::GetAllResourceOfType<Archtype>(AssetManagerSystem::ARCHTYPE_GUID);
			std::unordered_map<std::string, std::vector<int>> archtype_type_map{};
			for (int i = 0; i < archtype_arr.size(); i++)
			{
				//Store index of archtype instead of shared_ptr.
				//Separate archtypes into the different arrays based on model type.
				auto& pair = archtype_arr[i];
				archtype_type_map[pair.second->model_type].push_back(i);
				//Add the type to map if haven't. Default set to false to indicate not selected.
				if (selected_types.contains(pair.second->model_type)) continue;
				selected_types[pair.second->model_type] = false;
			}

			/*
				Display format:
				- Selected archtype types names on top.
				- Selectable List of model types (Characters, Background Obj etc.) on the left
				- Table of archtypes on the right
			*/
			//==Display selected type names at the top
			DrawSelectedTypes();

			//==Display type selection list in the left pane
			DrawTypeList(archtype_type_map);

			ImGui::SameLine();
			//==Display table of archtypes in the right pane
			DrawSelectedArchtypeTable(spacing, per_row, archtype_type_map, archtype_arr);

			//==Handle Archtype modification/creation events after everything else, to ensure that shared_ptr + index used by previous functions aren't invalidated.
			HandleArchtypeEvent();
		}

	}
	void ModelManager::Close() {
		LOGI("model manager closed");
	}

	//Creates archtype file, then reloads all archtypes.
	ModelManager::ModelErrorCode ModelManager::CreateArchtype(const std::string& name)
	{
		std::filesystem::path archtype_folder = AssetManagerSystem::GetAssetFolderPrefix() / AssetManagerSystem::GetFolderPath(AssetManagerSystem::ARCHTYPE);
		std::string archtype_extension = name + ".json";
		std::filesystem::path archtype_path = archtype_folder / archtype_extension;

		//Ensure the file doesn't already exist.
		if (std::filesystem::is_regular_file(archtype_path))
		{
			return ModelErrorCode::FILE_ALREADY_EXISTS;
		}
		//Create a basic archtype of an empty json.
		nlohmann::json empty_json = nlohmann::json::object();
		std::ofstream archtype_filestream{ archtype_path };
		archtype_filestream << empty_json.dump(1, '\t') << std::flush;

		//Update archtypes with this new change.
		AssetManagerSystem::ReloadAllResourcesOfType(AssetManagerSystem::ARCHTYPE, AssetManagerSystem::ARCHTYPE_GUID);
		return ModelErrorCode::SUCCESS;
	}
	//Creates archtype file of "name" from existing archtype, then reloads all archtypes.
	ModelManager::ModelErrorCode ModelManager::CopyArchtype(const std::string& name, const std::shared_ptr<Archtype> existing_archtype)
	{
		//Nullptr given.
		if (!existing_archtype)
		{
			LOGE("ModelManager::CopyArchtype Nullptr provided for archtype to copy.");
			return ModelErrorCode::INVALID_ARGUMENTS;
		}
		std::filesystem::path archtype_folder = AssetManagerSystem::GetAssetFolderPrefix() / AssetManagerSystem::GetFolderPath(AssetManagerSystem::ARCHTYPE);
		std::string archtype_extension = name + ".json";
		std::filesystem::path archtype_path = archtype_folder / archtype_extension;

		//Ensure the file doesn't already exist.
		if (std::filesystem::is_regular_file(archtype_path))
		{
			return ModelErrorCode::FILE_ALREADY_EXISTS;
		}

		//Serialize the archtype into the created file, thereby copying it.
		std::ofstream archtype_filestream{ archtype_path };
		archtype_filestream << existing_archtype->Serialize() << std::flush;

		//Update archtypes with this new change.
		AssetManagerSystem::ReloadAllResourcesOfType(AssetManagerSystem::ARCHTYPE, AssetManagerSystem::ARCHTYPE_GUID);
		return ModelErrorCode::SUCCESS;
	}
	//Rename archtype file, modifying guid.meta accordingly. 
	ModelManager::ModelErrorCode ModelManager::RenameArchtype(const std::string& old_name, const std::string& new_name)
	{
		std::filesystem::path archtype_folder = AssetManagerSystem::GetAssetFolderPrefix() / AssetManagerSystem::GetFolderPath(AssetManagerSystem::ARCHTYPE);
		std::filesystem::path old_archtype_path = archtype_folder / (old_name + ".json");
		std::filesystem::path new_archtype_path = archtype_folder / (new_name + ".json");

		//Ensure file exists.
		if (!std::filesystem::is_regular_file(old_archtype_path))
		{
			return ModelErrorCode::NOT_FOUND;
		}

		//Cannot rename if there's already another archtype file with the same name.
		if (std::filesystem::is_regular_file(new_archtype_path))
		{
			return ModelErrorCode::FILE_ALREADY_EXISTS;
		}

		//==Rename File
		std::error_code result{};
		std::filesystem::rename(old_archtype_path, new_archtype_path, result);
		if (result) return ModelErrorCode::UNKNOWN_FAILURE;

		//Modify existing guid <-> name maps.
		AssetManagerSystem::ModifyNameMappings(AssetManagerSystem::ARCHTYPE_GUID, old_name, new_name);

		//==Modify guid.meta file
		/*
			1. Get json object from file
			2. Delete old and add new archtype entry in json
			3. Write json object back to file.
		*/
		std::filesystem::path meta_path = archtype_folder / "guid.meta";
		if (!std::filesystem::is_regular_file(meta_path))
		{
			return ModelErrorCode::CANNOT_MODIFY_GUIDMETA;
		}
		std::string file_string = AssetManagerSystem::GetStringFromFile(meta_path);
		if (file_string.empty()) //Nothing to modify.
		{
			return ModelErrorCode::CANNOT_MODIFY_GUIDMETA;
		}
		nlohmann::json meta_json = nlohmann::json::parse(file_string);

		//==Delete old and add new archtype entry.
		if (!meta_json.contains(old_name)) return ModelErrorCode::SUCCESS; //No need to modify if there's no existing entry.
		xresource::instance_guid asset_guid = meta_json[old_name].get<uint64_t>();
		meta_json.erase(old_name);
		meta_json[new_name] = asset_guid.m_Value;

		//==Write back to file.
		std::ofstream meta_filestream{ meta_path };
		meta_filestream << meta_json.dump(1, '\t') << std::flush;

		return ModelErrorCode::SUCCESS;
	}
	//Deletes archtype file, modifying guid.meta accordingly. Archtypes are then reloaded. Returns -1 if file can't be deleted, -2 if guid.meta can't be modified.
	ModelManager::ModelErrorCode ModelManager::DeleteArchtype(const std::string& name)
	{
		std::filesystem::path archtype_folder = AssetManagerSystem::GetAssetFolderPrefix() / AssetManagerSystem::GetFolderPath(AssetManagerSystem::ARCHTYPE);
		std::string archtype_extension = name + ".json";
		std::filesystem::path archtype_path = archtype_folder / archtype_extension;

		//Ensure the file exists, delete if so.
		if (!std::filesystem::is_regular_file(archtype_path))
		{
			return ModelErrorCode::NOT_FOUND;
		}
		std::filesystem::remove(archtype_path);

		/*
			1. Get json object from file
			2. Delete archtype entry from json
			3. Write json object back to file.
		*/
		std::filesystem::path meta_path = archtype_folder / "guid.meta";
		if (!std::filesystem::is_regular_file(meta_path))
		{
			return ModelErrorCode::CANNOT_MODIFY_GUIDMETA;
		}
		std::string file_string = AssetManagerSystem::GetStringFromFile(meta_path);
		if (file_string.empty()) //Nothing to delete.
		{
			return ModelErrorCode::CANNOT_MODIFY_GUIDMETA;
		}
		nlohmann::json meta_json = nlohmann::json::parse(file_string);

		//==Delete entry and write back to file.
		if (meta_json.erase(name) == 0)
		{
			return ModelErrorCode::CANNOT_MODIFY_GUIDMETA;
		}
		std::ofstream meta_filestream{ meta_path };
		meta_filestream << meta_json.dump(1, '\t') << std::flush;

		//Update archtypes with this new change.
		AssetManagerSystem::ReloadAllResourcesOfType(AssetManagerSystem::ARCHTYPE, AssetManagerSystem::ARCHTYPE_GUID);
		return ModelErrorCode::SUCCESS;
	}

	//Only handles one event at a time. To be called at the end of Run().
	void ModelManager::HandleArchtypeEvent()
	{
		//Event data may be incomplete. If so, get further input.
		constexpr int buffer_size = 128;
		static char input_buffer[buffer_size] = "";
		//Only open a new popup if there's none currently.
		if (delayed_archtype_event.event != ArchtypeEvent::ArchtypeEvent::NONE && !delayed_archtype_event.is_popup_active)
		{
			delayed_archtype_event.is_popup_active = true;
			switch (delayed_archtype_event.event)
			{
			case ArchtypeEvent::ArchtypeEvent::COPY:
				ImGui::OpenPopup("Copy Archtype");
				break;
			case ArchtypeEvent::ArchtypeEvent::RENAME:
				ImGui::OpenPopup("Rename Archtype");
				break;
			case ArchtypeEvent::ArchtypeEvent::DELETE:
				ImGui::OpenPopup("Delete Archtype");
				break;
			case ArchtypeEvent::ArchtypeEvent::CREATE:
				ImGui::OpenPopup("Create Archtype");
				break;
			case ArchtypeEvent::ArchtypeEvent::CHANGE_MODEL_TYPE:
				ImGui::OpenPopup("Change Model Type");
				break;
			}
		}

		/*
			Helper Functions
		*/
		//Called after successful or canceled operation, to reset state for next event.
		auto clear_buffers_and_close_popup = []() {
			input_buffer[0] = '\0';
			delayed_archtype_event = {};
			ImGui::CloseCurrentPopup();
			};

		//Called to debug log errors.
		auto debug_log_errorcode = [](ModelErrorCode error) {

			switch (error)
			{
			case ModelErrorCode::INVALID_ARGUMENTS:
				LOGE("ModelManager::HandleArchtypeEvent Invalid Parameters passed when modifying/creating archtype.");
				break;
			case ModelErrorCode::FILE_ALREADY_EXISTS:
				LOGE("ModelManager::HandleArchtypeEvent File already exists when modifying/creating archtype.");
				break;
			case ModelErrorCode::NOT_FOUND:
				LOGE("ModelManager::HandleArchtypeEvent File not found when modifying/creating archtype.");
				break;
			case ModelErrorCode::CANNOT_MODIFY_GUIDMETA:
				LOGE("ModelManager::HandleArchtypeEvent Unable to update guid.meta when modifying/creating archtype");
				break;
			case ModelErrorCode::UNKNOWN_FAILURE:
				LOGE("ModelManager::HandleArchtypeEvent Unknown Error when modifying/creating archtype");
				break;
			}
			};


		/*==
			Create popup windows for Copy,Rename,Create,Delete,Change Model Type.
			These windows allow more input to be received from the user if necessary, and act as a final confirmation.
		*/

		//== Copy Archtype
		if (ImGui::BeginPopupModal("Copy Archtype"))
		{
			CALL_AT_END(imgui_endpopup, ImGui::EndPopup(););
			ImGui::InputText("New Archtype", input_buffer, buffer_size);
			if (ImGui::Button("Confirm"))
			{
				//Ensure input is non-empty, then use it + event data to call archtype operation.
				std::string input = input_buffer;
				ModelErrorCode result{};
				if (input.empty())
				{
					LOGE("ModelManager::HandleArchtypeEvent Archtype name cannot be blank when copying.");
				}
				else
				{
					result = CopyArchtype(input, delayed_archtype_event.archtype);
					if (result != ModelErrorCode::SUCCESS) debug_log_errorcode(result);
				}
				clear_buffers_and_close_popup();
			}
			if (ImGui::Button("Close"))
			{
				//Ignore event, user has chosen not to give enough information.
				clear_buffers_and_close_popup();
			}
			return;
		}



		//== Rename Archtype.
		if (ImGui::BeginPopupModal("Rename Archtype"))
		{
			CALL_AT_END(imgui_endpopup, ImGui::EndPopup(););
			ImGui::InputText("New Name", input_buffer, buffer_size);
			if (ImGui::Button("Confirm"))
			{
				//Ensure input is non-empty, then use it + event data to call archtype operation.
				std::string input = input_buffer;
				ModelErrorCode result{};
				if (input.empty())
				{
					LOGE("ModelManager::HandleArchtypeEvent Archtype name cannot be blank when renaming.");
				}
				else
				{
					result = RenameArchtype(delayed_archtype_event.current_name, input);
					if (result != ModelErrorCode::SUCCESS) debug_log_errorcode(result);
				}
				clear_buffers_and_close_popup();
			}
			if (ImGui::Button("Close"))
			{
				//Ignore event, user has chosen not to give enough information.
				clear_buffers_and_close_popup();
			}
			return;
		}

		//== Create Archtype.
		if (ImGui::BeginPopupModal("Create Archtype"))
		{
			CALL_AT_END(imgui_endpopup, ImGui::EndPopup(););
			ImGui::InputText("New Archtype", input_buffer, buffer_size);
			if (ImGui::Button("Confirm"))
			{
				//Ensure input is non-empty, then use it + event data to call archtype operation.
				std::string input = input_buffer;
				ModelErrorCode result{};
				if (input.empty())
				{
					LOGE("ModelManager::HandleArchtypeEvent Archtype name cannot be blank when creating.");
				}
				else
				{
					result = CreateArchtype(input);
					if (result != ModelErrorCode::SUCCESS) debug_log_errorcode(result);
				}
				clear_buffers_and_close_popup();
			}
			if (ImGui::Button("Close"))
			{
				//Ignore event, user has chosen not to give enough information.
				clear_buffers_and_close_popup();
			}
			return;
		}

		//== Delete Archtype.
		if (ImGui::BeginPopupModal("Delete Archtype"))
		{
			CALL_AT_END(imgui_endpopup, ImGui::EndPopup(););
			ImGui::TextWrapped("Are You Sure? There's No Going Back.");
			if (ImGui::Button("Confirm"))
			{
				ModelErrorCode result = DeleteArchtype(delayed_archtype_event.current_name);
				if (result != ModelErrorCode::SUCCESS) debug_log_errorcode(result);
				clear_buffers_and_close_popup();
			}
			if (ImGui::Button("Close"))
			{
				//Ignore event, user has chosen not to give enough information.
				clear_buffers_and_close_popup();
			}
			return;
		}

		//== Change Model Type
		if (ImGui::BeginPopupModal("Change Model Type"))
		{
			CALL_AT_END(imgui_endpopup, ImGui::EndPopup(););
			ImGui::InputText("Model Type", input_buffer, buffer_size);
			if (ImGui::Button("Confirm"))
			{
				//Input can be empty, since empty model type just means unlabelled.
				delayed_archtype_event.archtype->model_type = input_buffer;
				clear_buffers_and_close_popup();
			}
			if (ImGui::Button("Close"))
			{
				//Ignore event, user has chosen not to give enough information.
				clear_buffers_and_close_popup();
			}
			return;
		}

	}


}
#endif