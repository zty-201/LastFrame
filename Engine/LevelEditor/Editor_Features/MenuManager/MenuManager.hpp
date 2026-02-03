/*
*****************************************************************/
/*!
\file   MenuManager.hpp
\author Tan Yee Tong
\par    yeetong.t\@digipen.edu
\date   September 2025
\brief
This file allows the user to eit menu
Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* *******************************************************************/
#if defined(PLATFORM_WINDOWS) && defined(_DEBUG)
#ifndef MENUMANAGER_LFEATURE_HPP
#define MENUMANAGER_LFEATURE_HPP
#include <string>
#include <sstream>
#include "ISystem.hpp"
#include "entt/entity/registry.hpp"
#include "System_LevelEditor.hpp"
#include "LFeature.hpp"


namespace EngineFramework
{
    struct iEditorView; //forward declaration

    class MenuManager : public LFeature
    {
    public:

        MenuManager();
        ~MenuManager() override;

        void Initialize() override;
        void Run() override;
        void Close() override;

        static MenuManager& GetInstance();

        static bool is_menu_running; //if true means we are editing the menu, else no
       
        void ScanMenus();
        const std::string& GetCurrentMenu() const;
        void add_element(entt::entity e);
        void remove_element(entt::entity e);
    private:
        static inline MenuManager* this_system{};
        std::vector<std::string> menuList;
        std::string currentMenu;

        void create_new_menu();
        bool open_create_menu_popup;
    };
}

#endif
#endif