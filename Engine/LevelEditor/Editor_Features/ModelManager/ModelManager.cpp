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

	/*
		Draws the table of archtypes in the right pane.
		\param spacing per_row
		- The space between elements, number of elements per row of the table.
	*/
	void ModelManager::DrawSelectedArchtypeTable(int spacing, int per_row, std::unordered_map<std::string, std::vector<int>>& archtype_type_map, const std::vector<std::pair<xresource::instance_guid, std::shared_ptr<Archtype>>>& archtype_arr)
	{
		//==Display table of archtypes in the right pane
		const float avail = ImGui::GetContentRegionAvail().x;
		//Based on current window size, make the right pane 2/3 width by default.
		CALL_AT_END(imgui_child_right_pane, ImGui::EndChild(););
		ImGui::BeginChild("Child_RightPane", ImVec2(2 * avail / 3.f, 0), true, ImGuiChildFlags_ResizeX);
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
		- Model type
		- Name of archtype
		- Texture image if possible

		Popup Menu (Right click archtype button):
		- Enable dragdrop into scene (create instance with default transform at where camera is pointing, with option to choose normalized or world pos).
        - Creation of instance
		- Copy of existing archtype
		- Rename of archtype
		- Switch archtype model_type
        - Deletion of archtype
		*/
		
		entt::id_type texture_comp_id = entt::resolve<TextureComponent>().id();
		CALL_AT_END(imgui_endtable, ImGui::EndTable(););
		if (ImGui::BeginTable("model-grid", per_row, ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_PadOuterX | ImGuiTableFlags_NoBordersInBody))
		{
			int id = 0;
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

				int i = 0;
				for (int archtype_idx : selected_archtype_arr)
				{
					//==Setting of table element
					if (i % per_row == 0) ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(i % per_row);
					ImGui::PushID(id++);
					CALL_AT_END(imgui_popid, ImGui::PopID(););

					ImGui::BeginGroup();
					CALL_AT_END(imgui_endgroup, ImGui::EndGroup(););

					//==Accessing archtype name and texture (if available).
					auto& pair = archtype_arr[archtype_idx];
					xresource::instance_guid archtype_guid = pair.first;
					std::shared_ptr<Archtype> archtype = pair.second;

					std::string archtype_name = AssetManagerSystem::GetName(archtype_guid);
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
					ImGui::TextWrapped(archtype_name.c_str());
					i++;
				}
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
			const std::vector<std::pair<xresource::instance_guid, std::shared_ptr<Archtype>>> archtype_arr = AssetManagerSystem::GetAllResourceOfType<Archtype>(AssetManagerSystem::ARCHTYPE_GUID);
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
		}

	}
	void ModelManager::Close() {
		LOGI("model manager closed");
	}

}
#endif




/* for (auto asset : assets)
		 {
			 if (i % per_row == 0) ImGui::TableNextRow();
			 ImGui::TableSetColumnIndex(i % per_row);
			 ImGui::PushID(i);

			 ImGui::BeginGroup();
			 if (ImGui::ImageButton(asset.filename, (ImTextureID)0, ImVec2(cell_w, cell_w), { 0, 1 }, { 1, 0 }))
			 {

			 }

			 if (ImGui::BeginDragDropSource())
			 {
				 ImGui::SetDragDropPayload("Models", asset.filename, strlen(asset.filename) + 1);

				 ImGui::TextUnformatted(asset.filename);
				 ImGui::Image((ImTextureID)0, ImVec2(50, 50), ImVec2(0, 1), ImVec2(1, 0));

				 ImGui::EndDragDropSource();
			 }

			 ImGui::Text(asset.filename);

			 ImGui::EndGroup();

			 ImGui::PopID();

			 i++;*/

			 //}

/*
* O

	//Get the archtypes stored in asset manager.
std::vector<std::pair<xresource::instance_guid, std::shared_ptr<Archtype>>> archtype_arr = AssetManagerSystem::GetAllResourceOfType<Archtype>(AssetManagerSystem::ARCHTYPE_GUID);

int i = 0;

for (const auto& archtype_pair : archtype_arr)
{
	std::string archtype_name = AssetManagerSystem::GetName(archtype_pair.first);
	if (archtype_name.empty()) continue; //Cannot serialize an archtype with no name or value.

	ImGui::BeginGroup();

	if (ImGui::ImageButton(archtype_name.c_str(), (ImTextureID)0, ImVec2(image_width, image_width), { 0, 1 }, { 1, 0 }))
	{

	}

	if (ImGui::BeginDragDropSource())
	{
		ImGui::SetDragDropPayload("Models", archtype_name.c_str(), strlen(archtype_name.c_str()) + 1);

		ImGui::TextUnformatted(archtype_name.c_str());
		ImGui::Image((ImTextureID)0, ImVec2(50, 50), ImVec2(0, 1), ImVec2(1, 0));

		ImGui::EndDragDropSource();
	}

	ImGui::Text(archtype_name.c_str());

	ImGui::EndGroup();

	const bool lastInRow = ((i + 1) % per_row) == 0;
	if (!lastInRow) ImGui::SameLine(0.0f, (float)spacing);

	i++;

}

ImGui::Text("* Sample testing of model drag drop *");
ImGui::PopStyleVar();



*/
