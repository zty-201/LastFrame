/*
*****************************************************************/
/*!
\file   UISystem.cpp
\author Tan Yee Tong
\par    yeetong.t\@digipen.edu
\date   Jan 2026
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

#include <vector>
#include <string>
#include <functional>

#if defined(DEBUG) && defined(PLATFORM_WINDOWS)
#include "System_LevelEditor.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include "System_GameObjectSystem.hpp"
#endif

#include "AssetManager.hpp"

namespace EngineFramework
{
    // -------------------------------------------------------------------------
    // Cutscene sequence controller (dt-driven; swaps menus over time)
    // Lives in this .cpp only; UI_System owns the callbacks.
    // -------------------------------------------------------------------------
    namespace
    {
        struct CutsceneSequence
        {
            bool playing = false;
            std::size_t index = 0;
            float timer = 0.0f;
            float interval = 1.5f;

            std::vector<std::string> scenes;

            std::function<void(const std::string&)> showSceneFn; // should "swap" to this menu
            std::function<void()> onFinishedFn;

            void Start(std::vector<std::string> inScenes,
                float inInterval,
                std::function<void(const std::string&)> inShowSceneFn,
                std::function<void()> inOnFinishedFn = {})
            {
                scenes = std::move(inScenes);
                interval = (inInterval > 0.0f) ? inInterval : 1.5f;
                showSceneFn = std::move(inShowSceneFn);
                onFinishedFn = std::move(inOnFinishedFn);

                playing = !scenes.empty();
                index = 0;
                timer = 0.0f;

                if (playing && showSceneFn)
                {
                    showSceneFn(scenes[0]); // show first immediately
                }
            }

            void Stop()
            {
                playing = false;
            }

            void Update(float dt)
            {
                if (!playing || scenes.empty() || !showSceneFn) return;

                timer += dt;

                // Use while to handle low FPS spikes safely
                while (timer >= interval)
                {
                    timer -= interval;
                    ++index;

                    if (index >= scenes.size())
                    {
                        playing = false;
                        if (onFinishedFn) onFinishedFn();
                        return;
                    }

                    showSceneFn(scenes[index]);
                }
            }
        };

        CutsceneSequence gCutscene;
    }

    // -------------------------------------------------------------------------
    // Existing code
    // -------------------------------------------------------------------------
    std::unordered_map<std::string, std::pair<std::unique_ptr<UI_Menu>, bool>> UI_System::Menu_Mapping;

    void UI_System::ErrorCallback(int error, const char* description)
    {
        (void)error;
        UNREFERENCED_PARAMETER(description);
        //LOGE("GLFW Error: %s", description);
    }

    void UI_System::Initialize()
    {
        std::cout << "=========================================\n";
        std::cout << "        UI_System::Initialize()          \n";
        std::cout << "=========================================\n";

        DeserializeAllOverlays();

        for (auto& [name, menu] : Menu_Mapping)
        {
            if (menu.first)
            {
                menu.first->Initialize();
                menu.second = false;
            }
        }

        current_menu = ""; // none, i havent set default yet

        Load_Start_Menu();
    }

    void UI_System::Clean_All()
    {
        for (auto& [name, menu] : Menu_Mapping)
        {
            if (menu.first)
            {
                menu.first->removeAllEntities_Reg(); //only update those active ones
            }
        }

        Menu_Mapping.clear();
        current_menu.clear();

        // also stop any running cutscene sequence
        gCutscene.Stop();
    }

    void UI_System::DeserializeAllOverlays()
    {
        Menu_Mapping.clear();
        std::vector<std::filesystem::path> valid_json_overlays =
            AssetManagerSystem::ListFilesInDirectory("Extern/Overlays", { ".json" });

        for (const auto& entry : valid_json_overlays)
        {
            const std::string menuName = entry.stem().string();
            DeserializeOverlay(menuName);
        }
    }

    bool UI_System::DeserializeOverlay(std::string menuName)
    {
        auto menu = std::make_unique<UI_Menu>(menuName);

        if (!menu->Deseralise(menuName))
        {
            return false;
        }

        Menu_Mapping.emplace(menuName, std::pair(std::move(menu), false));
        return true;
    }

    void UI_System::Update(float deltaTime)
    {
        // 1) Tick cutscene sequence FIRST (so it can swap menus at the right moment)
        gCutscene.Update(deltaTime);

        // 2) Update only active menus
        for (auto& [name, menu] : Menu_Mapping)
        {
            if (menu.first)
            {
                if (menu.second)
                {
                    menu.first->Update(); //only update those active ones
                }
            }
        }
    }

    void UI_System::PostUpdate(float deltaTime)
    {
        (void)deltaTime;
    }

    void UI_System::add_entity_fromMenu(std::string menu, entt::entity e)
    {
        auto it = Menu_Mapping.find(menu);

        if (it != Menu_Mapping.end() && it->second.first)
        {
            it->second.first->Add_Element(e);
        }
    }

    void UI_System::remove_entity_fromMenu(std::string menu, entt::entity e)
    {
        auto it = Menu_Mapping.find(menu);

        if (it != Menu_Mapping.end() && it->second.first)
        {
            it->second.first->Remove_Element(e);
        }
    }

#ifdef PLATFORM_WINDOWS //No need to create on android.

    std::unordered_set<entt::entity> UI_System::Seralise_AllMenus()
    {
        std::unordered_set<entt::entity> combined;

        for (auto& [name, menu] : Menu_Mapping)
        {
            if (menu.first)
            {
                std::unordered_set<entt::entity> entities = menu.first->Seralise();
                combined.insert(entities.begin(), entities.end());
                menu.second = false; // set to false
            }
        }

        return combined;
    }

    void UI_System::create_overlay(std::string menuName)
    {
        std::filesystem::path path =
            AssetManagerSystem::GetInstance().GetAssetFolderPrefix().string() +
            "Extern/Overlays/" + menuName + ".json";

        // Only create if it doesn't exist
        if (!std::filesystem::exists(path))
        {
            std::ofstream out(path);
            if (out.is_open())
            {
                out << "{}";
                out.close();
            }
        }

        auto menu = std::make_unique<UI_Menu>(menuName);
        menu->Initialize();
        Menu_Mapping.emplace(menuName, std::pair(std::move(menu), false));
    }
#endif

    std::vector<entt::entity> UI_System::getMenu_Entities(std::string menuname)
    {
        auto it = Menu_Mapping.find(menuname);

        if (it != Menu_Mapping.end() && it->second.first)
        {
            return it->second.first->get_Elements();
        }

        return {};
    }

    std::unordered_set<entt::entity> UI_System::getAllMenu_Entities()
    {
        std::unordered_set<entt::entity> set;

        for (auto& [name, menu] : Menu_Mapping)
        {
            if (!menu.first) continue;

            const auto& elems = menu.first->get_Elements();
            set.insert(elems.begin(), elems.end());
        }

        return set;
    }

    void UI_System::Load_Start_Menu()
    {
        displayMenu("StartMenu");
    }

    void UI_System::displayMenu(std::string menuname)
    {
        std::cout << "[UI_System] displayMenu('" << menuname << "')\n";

        auto it = Menu_Mapping.find(menuname);

        if (it != Menu_Mapping.end() && it->second.first)
        {
            std::cout << "[UI_System] displayMenu success: " << menuname << "\n";

            it->second.first->draw_elements();
            current_menu = menuname;
            it->second.second = true;
        }
    }

    void UI_System::closeMenu(std::string menuname)
    {
        auto it = Menu_Mapping.find(menuname);

        if (it != Menu_Mapping.end() && it->second.first)
        {
            it->second.first->draw_elements(false);
            current_menu = "";
            it->second.second = false;
        }
    }

    void UI_System::closeMenu()
    {
        auto it = Menu_Mapping.find(current_menu);

        if (it != Menu_Mapping.end() && it->second.first)
        {
            it->second.first->draw_elements(false);
            current_menu = "";
            it->second.second = false;
        }
    }

    void UI_System::closeAllMenu()
    {
        for (auto& [name, menu] : Menu_Mapping)
        {
            if (menu.first)
            {
                menu.first->draw_elements(false);
                menu.second = false;
            }
        }

        current_menu.clear();
    }

    bool UI_System::IsMenuActive(const std::string& menuname) const
    {
        auto it = Menu_Mapping.find(menuname);
        if (it == Menu_Mapping.end() || !it->second.first)
            return false;
        return it->second.second;
    }

    // -------------------------------------------------------------------------
    // NEW: Cutscene API (call this from your Play button)
    // -------------------------------------------------------------------------

    bool UI_System::IsCutscenePlaying() const
    {
        return gCutscene.playing;
    }

    void UI_System::StopCutsceneSequence(bool closeAll)
    {
        gCutscene.Stop();
        if (closeAll)
        {
            closeAllMenu();
        }
    }

    void UI_System::StartCutsceneSequence(const std::vector<std::string>& sceneMenus,
        float intervalSeconds,
        bool closeOthersFirst)
    {
        std::cout << "[UI_System] StartCutsceneSequence() scenes:\n";
        for (auto& s : sceneMenus)
            std::cout << "  - " << s << (Menu_Mapping.count(s) ? " (FOUND)\n" : " (MISSING)\n");

        // Make a local copy for the controller (it stores a vector internally)
        std::vector<std::string> scenes = sceneMenus;

        gCutscene.Start(
            std::move(scenes),
            intervalSeconds,
            // show function: make it a real "swap"
            [this, closeOthersFirst](const std::string& menuName)
            {
                if (closeOthersFirst)
                    closeAllMenu();
                displayMenu(menuName);
            },
            // finished function (optional): what happens after cutscene_4?
            [this]()
            {
                // Example: go to main menu / start game menu
                // You can change this to whatever you want:
                // displayMenu("MainMenu");
                // OR trigger scene load / gameplay start etc.
                // For now: return to StartMenu
                closeAllMenu();
                //displayMenu("StartMenu");
            }
        );
    }

    void UI_System::StartPseudoCutscene()
    {
        std::cout << "[UI_System] StartPseudoCutscene() called\n";

        // Your described chain: cutscene_1 -> cutscene_4, 1.5s interval
        StartCutsceneSequence(
            { "Cutscene_1", "Cutscene_2", "Cutscene_3", "Cutscene_4" },
            1.5f,
            true
        );
    }

}
