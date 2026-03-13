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

#endif
#include "System_GameObjectSystem.hpp"
#include "AssetManager.hpp"
#include "JoystickComponent.h"
#include "UIComponent.h"
#include "Input/InputManager.hpp"
#include "NameComponent.h"
#include "Script.hpp"

namespace EngineFramework
{
    // -------------------------------------------------------------------------
    // Cutscene sequence controller (dt-driven; swaps menus over time)
    // Lives in this .cpp only; UI_System owns the callbacks.
    // -------------------------------------------------------------------------
    namespace
    {
        static void SetMenuLuminance(const std::string& menuName, float luminance)
        {
            auto it = EngineFramework::UI_System::Menu_Mapping.find(menuName);
            if (it == EngineFramework::UI_System::Menu_Mapping.end() || !it->second.first)
                return;

            auto entities = it->second.first->get_Elements();
            for (auto e : entities)
            {
                if (registry.valid(e) && registry.all_of<UIComponent>(e))
                {
                    auto& ui = registry.get<UIComponent>(e);
                    if (!ui.lock_luminance)
                    {
                        ui.luminance = luminance;
                    }
                }
            }
        }

        static void ResetMenuLuminance(const std::string& menuName)
        {
            SetMenuLuminance(menuName, 1.0f);
        }

        static void SetMenuSubtitleReveal(const std::string& menuName, float progress)
        {
            auto it = EngineFramework::UI_System::Menu_Mapping.find(menuName);
            if (it == EngineFramework::UI_System::Menu_Mapping.end() || !it->second.first)
                return;

            progress = std::clamp(progress, 0.0f, 1.0f);

            auto entities = it->second.first->get_Elements();
            for (auto e : entities)
            {
                if (registry.valid(e) && registry.all_of<UIComponent>(e))
                {
                    auto& ui = registry.get<UIComponent>(e);

                    // Only apply to subtitle-like elements if you want all elements in the menu to reveal
                    // then leave this unconditional.
                    if (ui.use_subtitle_reveal)
                    {
                        ui.subtitle_reveal_progress = progress;
                        std::cout << "[SubtitleReveal] " << menuName
                            << " progress = " << progress << "\n";
                    }
                }
            }
        }

        //static void ResetMenuSubtitleReveal(const std::string& menuName)
        //{
        //    SetMenuSubtitleReveal(menuName, 1.0f);
        //}

        struct CutsceneSequence
        {
            bool playing = false;
            std::size_t index = 0;
            float timer = 0.0f;
            float interval = 2.0f;

            // Per-screen fade timing inside one interval
            float fadeInTime = 0.5f;
            float fadeOutTime = 0.5f;

            std::vector<std::string> scenes;

            std::function<void(const std::string&)> showSceneFn;
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

                // Clamp fade values so they always fit inside the interval
                float maxFade = interval * 0.5f;
                if (fadeInTime > maxFade) fadeInTime = maxFade;
                if (fadeOutTime > maxFade) fadeOutTime = maxFade;

                if (playing && showSceneFn)
                {
                    showSceneFn(scenes[0]);
                    SetMenuLuminance(scenes[0], 0.0f); // start dark, fade in
                }
            }

            void Skip()
            {
                if (!playing)
                    return;

                if (index < scenes.size())
                {
                    ResetMenuLuminance(scenes[index]);
                }

                playing = false;
                timer = 0.0f;
                index = scenes.size();

                if (onFinishedFn)
                {
                    onFinishedFn();
                }
            }

            void Stop()
            {
                if (playing && index < scenes.size())
                {
                    ResetMenuLuminance(scenes[index]);
                }

                playing = false;
            }

            void Update(float dt)
            {
                if (!playing || scenes.empty() || !showSceneFn)
                    return;

                timer += dt;

                // Animate luminance of the current scene
                const std::string& currentScene = scenes[index];

                // Subtitle reveal timing
                float subtitleDuration = 1.0f;
                float subtitleProgress = std::clamp(timer / subtitleDuration, 0.0f, 1.0f);
                SetMenuSubtitleReveal(currentScene, subtitleProgress);

                float luminance = 1.0f;

                if (timer < fadeInTime)
                {
                    // Fade in: 0 -> 1
                    luminance = timer / fadeInTime;
                }
                else if (timer > (interval - fadeOutTime))
                {
                    // Fade out: 1 -> 0
                    float t = (timer - (interval - fadeOutTime)) / fadeOutTime;
                    luminance = 1.0f - t;
                }
                else
                {
                    // Hold
                    luminance = 1.0f;
                }

                luminance = std::clamp(luminance, 0.0f, 1.0f);
                SetMenuLuminance(currentScene, luminance);

                while (timer >= interval)
                {
                    timer -= interval;

                    // Ensure previous scene is restored
                    ResetMenuLuminance(scenes[index]);

                    ++index;

                    if (index >= scenes.size())
                    {
                        playing = false;
                        if (onFinishedFn) onFinishedFn();
                        return;
                    }

                    showSceneFn(scenes[index]);
                    SetMenuLuminance(scenes[index], 0.0f); // next scene begins faded out
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

        //Load_Start_Menu();
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
        // Allow skipping cutscenes with Space (desktop) or tap anywhere (Android)
        if (gCutscene.playing)
        {
            bool shouldSkip = false;

            // Desktop / existing input
            if (InputManager::GetInstance().GetAction("JUMP"))
            {
                shouldSkip = true;
            }

#if defined(PLATFORM_ANDROID)
            // Tap-anywhere skip (rising edge only)
            static bool wasTouchDownLastFrame = false;
            bool isTouchDownThisFrame = InputManager::GetInstance().GetMouseButton(0);

            if (isTouchDownThisFrame && !wasTouchDownLastFrame)
            {
                shouldSkip = true;
            }

            wasTouchDownLastFrame = isTouchDownThisFrame;
#endif

            if (shouldSkip)
            {
                gCutscene.Skip();
            }
        }

        // 1) Tick cutscene sequence FIRST
        gCutscene.Update(deltaTime);

        // 2) Update only active menus
        for (auto& [name, menu] : Menu_Mapping)
        {
            if (menu.first)
            {
                if (menu.second)
                {
                    menu.first->Update();
                    if (name == "AndroidInputOverlay")
                    {
                        update_Joysticks();
                    }
                }
            }
        }
    }

    void UI_System::update_Joysticks()
    {
        auto view = GameObjectSystem::GetRegistry().view<JoystickComponent>();

        std::pair<double, double> mousePos = InputManager::GetInstance().GetMousePosition();
        bool isTouchDown = InputManager::GetInstance().GetMouseButton(0);

        float screenWidth = (float)InputManager::GetInstance().GetScreenSize().first;
        float screenHeight = (float)InputManager::GetInstance().GetScreenSize().second;
        auto& lua = Engine::ScriptSystem::GetLuaState();

        for (auto [entity, joystick] : view.each())
        {
            if (!registry.valid(joystick.knobEntity))
            {
                for (auto [e, nameComp] : registry.view<NameComponent>().each())
                {
                    if (nameComp.name == joystick.knobTargetName)
                    {
                        joystick.knobEntity = e;
                        break;
                    }
                }
            }

            if (registry.all_of<UIComponent>(entity))
            {
                auto& bgUI = registry.get<UIComponent>(entity);
                if (bgUI.is_NDC)
                {
                    float trueCenterX = bgUI.position_NDC.x;
                    float trueCenterY = bgUI.position_NDC.y;

                    joystick.baseCenter.x = ((trueCenterX + 1.f) / 2.f) * screenWidth;
                    joystick.baseCenter.y = ((1.f - trueCenterY) / 2.f) * screenHeight;
                }
                else
                {
                    joystick.baseCenter.x = bgUI.position.x;
                    joystick.baseCenter.y = bgUI.position.y;
                }
            }

            glm::vec2 currentOffset{ 0.f, 0.f };
            if (isTouchDown)
            {
                //Check if clicked inside joystick area to start dragging
                if (!joystick.isBeingDragged)
                {
                    float dist = glm::distance(glm::vec2(mousePos.first, mousePos.second), joystick.baseCenter);
                    if (dist <= joystick.maxRadius)
                    {
                        joystick.isBeingDragged = true;
                    }
                }

                if (joystick.isBeingDragged)
                {
                    glm::vec2 touchPos = { (float)mousePos.first, (float)mousePos.second };
                    currentOffset = touchPos - joystick.baseCenter;

                    //Clamp distance
                    float dist = glm::length(currentOffset);
                    if (dist > joystick.maxRadius)
                    {
                        currentOffset = (currentOffset / dist) * joystick.maxRadius;
                    }

                    joystick.outputX = currentOffset.x / joystick.maxRadius;
                    joystick.outputY = currentOffset.y / joystick.maxRadius;
                }
            }
            else
            {
                //Release touch -> snap back to center
                if (joystick.isBeingDragged)
                {
                    joystick.isBeingDragged = false;
                    joystick.outputX = 0.f;
                    joystick.outputY = 0.f;
                }
            }

            std::string luaKeyX = joystick.knobTargetName + "_X";
            std::string luaKeyY = joystick.knobTargetName + "_Y";
            lua["SHARED"]["UI"][luaKeyX] = joystick.outputX;
            lua["SHARED"]["UI"][luaKeyY] = joystick.outputY;

            if (registry.valid(joystick.knobEntity) && registry.all_of<UIComponent>(joystick.knobEntity))
            {
                auto& knobUI = registry.get<UIComponent>(joystick.knobEntity);
                glm::vec2 targetPixelPos = joystick.baseCenter + currentOffset;

                if (knobUI.is_NDC)
                {
                    float targetNdcX = (targetPixelPos.x / screenWidth) * 2.f - 1.f;
                    float targetNdcY = 1.f - (targetPixelPos.y / screenHeight) * 2.f;

                    knobUI.position_NDC.x = targetNdcX;
                    knobUI.position_NDC.y = targetNdcY;
                }
                else
                {
                    knobUI.position.x = targetPixelPos.x;
                    knobUI.position.y = targetPixelPos.y;
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
        //displayMenu("AndroidInputOverlay"); //remember to set if end flags for android build.
    }

    void UI_System::displayMenu(std::string menuname)
    {
        //std::cout << "[UI_System] displayMenu('" << menuname << "')\n";

        auto it = Menu_Mapping.find(menuname);

        if (it != Menu_Mapping.end() && it->second.first)
        {
            //std::cout << "[UI_System] displayMenu success: " << menuname << "\n";

            it->second.first->Initialize(); // Reset button states before showing
            it->second.first->draw_elements();
            current_menu = menuname;
            it->second.second = true;
        }
    }

    void UI_System::displayMenu_InvisibleLayer(std::string menuname)
    {
        //std::cout << "[UI_System] displayMenu('" << menuname << "')\n";

        auto it = Menu_Mapping.find(menuname);

        if (it != Menu_Mapping.end() && it->second.first)
        {
            std::cout << "[UI_System] displayMenu invisible success: " << menuname << "\n";

            it->second.first->draw_elements(false);
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

    bool UI_System::is_MenuOpen(std::string menuname){

        auto it = Menu_Mapping.find(menuname);

        if (it != Menu_Mapping.end() && it->second.first)
        {
            return it->second.second;
        }

        return false;
    }

    void UI_System::closeMenu()
    {
        auto it = Menu_Mapping.find(current_menu);

        if (it != Menu_Mapping.end() && it->second.first)
        {
            it->second.first->draw_elements(false);
            current_menu = "";
            it->second.second = false;
            it->second.first->set_AllbuttonState();
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
                menu.first->set_AllbuttonState();
            }
        }

        current_menu.clear();

        std::cout << "=========================== CLOSEEEEE ALLLLLL MENU () ======================================\n";
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
            //displayMenu("Android_Inventory_Buttons");

#if defined(PLATFORM_ANDROID)
            displayMenu_InvisibleLayer("Android_Inventory_Buttons");
            displayMenu("AndroidInputOverlay");
#endif
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
#if defined(PLATFORM_ANDROID)
                displayMenu("Android_Inventory_Buttons");
                displayMenu("AndroidInputOverlay");
#endif
            }
        );

        
    }

    void UI_System::StartSplashSequence()
    {
        std::cout << "[UI_System] StartSplashSequence() called\n";

        StartCutsceneSequence(
            { "Splash_1", "Splash_fmod", "Splash_2", "Splash_3" },
            2.5f,
            true
        );
    }

    //No longer in use?
    void UI_System::StartPseudoCutscene()
    {
        std::cout << "[UI_System] StartPseudoCutscene() called\n";

        // Your described chain: cutscene_1 -> cutscene_4, 1.5s interval
        StartCutsceneSequence(
            { "Cutscene_1", "Cutscene_2", "Cutscene_3", "Cutscene_4", "Cutscene_5"},
            1.5f,
            true
        );
    }

}
