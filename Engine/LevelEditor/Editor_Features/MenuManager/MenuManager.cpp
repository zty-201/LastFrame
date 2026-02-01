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
	}

	MenuManager& MenuManager::GetInstance()
	{
		return *this_system;
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
		if (!is_enabled) return; // only open if toggled

		if (ImGui::Begin("Menu Manager", &is_enabled))
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
				}
			}
			ImGui::Separator();

			if (ImGui::Button("Create New Menu"))
			{			

				LOGD("New Menu created, bleh!");
			}

			ImGui::Separator();

			if (ImGui::Button("Save/Close Menu Editor and Return to Scene"))
			{
				is_enabled = false;
				is_menu_running = false;
				currentMenu.clear();
				UI_System::GetInstance().closeAllMenu();
				UI_System::GetInstance().Seralise_AllMenus();

				LOGD("Exited Menu Editor, returning to scene");
			}

			



		}
		ImGui::End();
	}

	void MenuManager::Close()
	{
		LOGI("Menu Manager closed");
	}
}
#endif