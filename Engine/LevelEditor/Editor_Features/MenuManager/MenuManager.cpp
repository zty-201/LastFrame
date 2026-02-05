/*
*****************************************************************/
/*!
\file   SceneManager.cpp
\author Tang Yeun, Dean
\par    y.tang\@digipen.edu
\date   September 2025
\brief
This file allows the user to change scenes / levels
Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* *******************************************************************/
#if defined(PLATFORM_WINDOWS) && defined(_DEBUG)
#include "pch.hpp"
#include "MenuManager.hpp"
#include "System_GameObjectSystem.hpp"
#include <filesystem>
#include <iostream>
#include "AssetManager.hpp"
#include "Screen.hpp"
#include "UISystem.hpp"


namespace EngineFramework
{
	//defualt scene to load
	
	bool MenuManager::is_menu_running = false;

	MenuManager::MenuManager() {
		this_system = this;
		LOGI("Lfeature: MenuManager CREATED");
	}
	MenuManager::~MenuManager() {}

	void MenuManager::Initialize()
	{
		ScanMenus();	
		currentMenu = "";
		is_menu_running = false;
		open_create_menu_popup = false;

		UI_System::GetInstance().Load_Start_Menu(); //if need uncomment
	}

	MenuManager& MenuManager::GetInstance()
	{
		return *this_system;
	}

	void MenuManager::add_element(entt::entity e) {
		UI_System::GetInstance().add_entity_fromMenu(currentMenu, e);
	}

	void MenuManager::remove_element(entt::entity e) {
		UI_System::GetInstance().remove_entity_fromMenu(currentMenu,e);
	}

	void MenuManager::ScanMenus()
	{
		menuList.clear();

		for (auto& [menu_name, obj] : UI_System::Menu_Mapping) {

			if (!menu_name.empty()) {
				menuList.emplace_back(menu_name);
			}
		}

	}

	const std::string& MenuManager::GetCurrentMenu() const
	{
		return currentMenu;
	}

	void MenuManager::Run()
	{
		/*
		* simulation must be closed
		*/
		if (Screen::GetInstance().is_simulator_running) {
			is_enabled = false;
			return;
		}
		
		if (!is_enabled) return; // only open if toggled		
		
		if (ImGui::Begin("Menu Manager"))
		{
			ImGui::Text("Available Menu:");
			ImGui::Separator();
			for (const auto& menu : menuList)
			{
				if (ImGui::Selectable(menu.c_str()))
				{
					LOGD("Selected Menu: %s", menu.c_str());
					
					currentMenu = menu;
					is_menu_running = true;
					UI_System::GetInstance().closeAllMenu();
					UI_System::GetInstance().displayMenu(menu);

					//std::cout << "========= LIST OF ENTITIES HERE ============\n";
					auto list = UI_System::GetInstance().getMenu_Entities(menu);

					for (auto obj: list) {

						if (!registry.valid(obj)) {
							//std::cout << "Entity: " << (int)obj  << " | is not valid !registry.valid(entity) \n";		
							
						}
						else {
							//std::cout << "Entity: " << (int)obj << " |  is valid registry.valid(entity) \n";
						}


					}
				}
			}
			ImGui::Separator();

			if (ImGui::Button("Create New Menu"))
			{
				open_create_menu_popup = true;          // request popup open this frame
				ImGui::OpenPopup("Create New Menu");    // open modal

				//LOGD("New Menu created, bleh!");
			}

			ImGui::Separator();

			if (ImGui::Button("Close Menu Editor and Return to Scene"))
			{
				close_menuEditor();
			}

			create_new_menu();


		}
		ImGui::End();
	}

	void MenuManager::close_menuEditor() {

		is_enabled = false;
		is_menu_running = false;
		currentMenu.clear();
		UI_System::GetInstance().closeAllMenu();
		//UI_System::GetInstance().Seralise_AllMenus();

		LOGD("Exited Menu Editor, returning to scene");

	}

	void MenuManager::create_new_menu() {

		//popup modal logic
		if (open_create_menu_popup)
		{
			static char new_menu_name[128] = "";

			ImGui::SetNextWindowSize(ImVec2(420, 0), ImGuiCond_Always);
			ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(),
				ImGuiCond_Always, ImVec2(0.5f, 0.5f));

			bool keep_open = true;
			if (ImGui::BeginPopupModal("Create New Menu", &keep_open, ImGuiWindowFlags_AlwaysAutoResize)) {
				ImGui::Text("Enter a menu name:");
				ImGui::InputText("##MenuName", new_menu_name, IM_ARRAYSIZE(new_menu_name));
				ImGui::Separator();


				if (ImGui::Button("Create"))
				{

					open_create_menu_popup = false;
					UI_System::GetInstance().create_overlay(std::string(new_menu_name));
					ScanMenus(); //rescan again
					new_menu_name[0] = '\0';
					ImGui::CloseCurrentPopup();
				}

				ImGui::SameLine();

				// Cancel
				if (ImGui::Button("Cancel"))
				{
					new_menu_name[0] = '\0';
					open_create_menu_popup = false;
					ImGui::CloseCurrentPopup();
				}

				ImGui::EndPopup();
			}

			if (!keep_open)
			{
				new_menu_name[0] = '\0';
				open_create_menu_popup = false;
				ImGui::CloseCurrentPopup();
			}
		}

	}

	void MenuManager::Close()
	{
		LOGI("Menu Manager closed");
	}
}
#endif