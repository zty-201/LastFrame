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
#include "GraphicsSystem.h"
#include "Screen.hpp"


namespace EngineFramework
{
    Screen::Screen() {
        LOGI("Lfeature: Screen CREATED");
    }
    Screen::~Screen() {

    }
    void Screen::Initialize() {

        GraphicsSystem::GetInstance().GetScene().get()->camerasSetType(WALKING);
    }
    void Screen::Run() {

        
        ImGui::SetNextWindowSize(ImVec2(420, 260), ImGuiCond_FirstUseEver);

        ImGui::Begin("level loaded: <name>", &is_enabled);
              
        display_screen();

        if (bool hovered = ImGui::IsItemHovered()) {
            camera_controls_in_screen_only();
        }
        camera_controls_outside_screen();

        ImGui::End();

    }
    void Screen::Close() {
        LOGI("Screen closed");
    }

    void Screen::display_screen() {

        //ImVec2 imagepos = ImGui::GetCursorScreenPos();
        ImVec2 available_size = ImGui::GetContentRegionAvail();
        static ImVec2 available_size_old = available_size;
        static bool first_init = true;

        //Resize window on first initialization.
        if (first_init) {
            Event_Window_Resized window_resize;
            window_resize.width = static_cast<int>(available_size.x);
            window_resize.height = static_cast<int>(available_size.y);
            event_dispatcher.trigger<Event_Window_Resized>(std::move(window_resize));
            first_init = false;
        }

        //only do this when needed.
        if (available_size.x != available_size_old.x || available_size.y != available_size_old.y)
        {
            //resize when the screen size changes		
            //std::cout << "x: " << available_size.x << "y: " << available_size.y << std::endl;
            Event_Window_Resized window_resize;
            window_resize.width = static_cast<int>(available_size.x);
            window_resize.height = static_cast<int>(available_size.y);
            event_dispatcher.trigger<Event_Window_Resized>(std::move(window_resize));
        }

        ImGui::Image((void*)(intptr_t)GraphicsSystem::GetInstance().GetSceneTextureID(),
            available_size,
            ImVec2(0, 1), ImVec2(1, 0));

        available_size_old = available_size;
    }

    void Screen::camera_controls_outside_screen() {

        float speed = 0.05f;

        if (ImGui::IsKeyDown(ImGuiKey_UpArrow)) {
            //up vector
            GraphicsSystem::GetInstance().GetScene().get()->camerasSetPos_andTarget(glm::vec3(0, speed, 0));
            //std::cout << "UP !" << std::endl;
        }
        if (ImGui::IsKeyDown(ImGuiKey_DownArrow)) {
            //down vector
            //std::cout << "DOWN !" << std::endl;
            GraphicsSystem::GetInstance().GetScene().get()->camerasSetPos_andTarget(glm::vec3(0, -speed, 0));
        }
        if (ImGui::IsKeyDown(ImGuiKey_LeftArrow)) {
            //left vector
            //std::cout << "LEFT !" << std::endl;
            GraphicsSystem::GetInstance().GetScene().get()->camerasSetPos_andTarget(glm::vec3(-speed, 0, 0));
        }
        if (ImGui::IsKeyDown(ImGuiKey_RightArrow)) {
            //right vector
            //std::cout << "RIGHT !" << std::endl;
            GraphicsSystem::GetInstance().GetScene().get()->camerasSetPos_andTarget(glm::vec3(speed, 0, 0));
        }

    }
    void Screen::camera_controls_in_screen_only() {

        ImGuiIO& io = ImGui::GetIO();
        ImVec2 Mouse_delta = io.MouseDelta;
        float Mouse_Scroll = io.MouseWheel;

        auto& pScene = GraphicsSystem::GetInstance().GetScene();

        //right mouse is down means = i am dragging the screen
        if (io.MouseDown[1]) {
            pScene->camerasOnCursor(Mouse_delta.x, Mouse_delta.y);
            //std::cout << "Mouse_delta x: " << Mouse_delta.x << " y: " << Mouse_delta.y << std::endl;
        }

        //moving forward/backwards
        if (Mouse_Scroll != 0.f) {
            //std::cout << "scroll: " << Mouse_Scroll << std::endl;
            GraphicsSystem::GetInstance().GetScene().get()->camerasSetPos_andTarget(glm::vec3(0, 0, Mouse_Scroll));
        }

    }

}
#endif
