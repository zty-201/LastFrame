/*
*****************************************************************/
/*!
\file   InputManager.cpp
\author Tang Yeun, Dean
\par    y.tang\@digipen.edu
\date   September 2025
\brief
This file contains the definitions of the functions that are needed for
handling the inputs in the system.
Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* *******************************************************************/
#include "pch.hpp"
#include "global.hpp"
#include "UISystem.hpp"
#include "GraphicsSystem.h"


#if defined(DEBUG) && defined(PLATFORM_WINDOWS)
#include "System_LevelEditor.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>

#endif

#include "AssetManager.hpp"

namespace EngineFramework
{
#if defined(DEBUG) && defined(PLATFORM_WINDOWS)
    std::vector<entt::entity> UI_Menu::AllElements_in_this_overlay;

    void UI_Menu::Initialize() {

    }
    void UI_Menu::Update() {

    }
    void UI_Menu::Clean() {

    }
    void UI_Menu::Seralise() {

    }
    void UI_Menu::Deseralise() {

    }
    void UI_Menu::Add_Element(entt::entity e) {

        UNREFERENCED_PARAMETER(e);

    }
    void UI_Menu::Remove_Element(entt::entity e) {
        UNREFERENCED_PARAMETER(e);
    }

    void UI_Menu::update_Buttons() {

    }
    void UI_Menu::update_Button_State() {

    }
    void UI_Menu::Trigger_Button_Event() {

    }

    void UI_Menu::draw_elements() {

    }
#endif
}
