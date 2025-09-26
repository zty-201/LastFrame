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
#include "ImGuizmo.h"
//#include <iostream>
namespace EngineFramework
{
    bool Screen::control_open = false;
    bool Screen::simulator_running = false;


    Screen::Screen() {
        LOGI("Lfeature: Screen CREATED");
    }
    Screen::~Screen() {

    }
    void Screen::Initialize() {

        GraphicsSystem::GetInstance().GetScene().get()->camerasSetType(WALKING);
    }
    void Screen::Run() {

        ImGui::PushStyleColor(ImGuiCol_MenuBarBg, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));  // White bar
        ImGui::SetNextWindowSize(ImVec2(800, 800), ImGuiCond_Once);  // Width: 300,500

        if (ImGui::Begin("level loaded: <name>", &is_enabled, ImGuiWindowFlags_MenuBar)) {

            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));      // Black text
            if (ImGui::BeginMenuBar()) {

                if (ImGui::MenuItem("Run Simulator", NULL, &simulator_running)) {
                    //std::cout << "Run Simulator\n";                   
                }

                ImGui::SameLine(0, 50.0f); // The second argument controls the spacing in pixels


                if (ImGui::MenuItem("Interface Controls", NULL, &control_open)) {
                    //std::cout << "Controls\n";                   
                }

                ImGui::EndMenuBar();

            }

            ImGui::PopStyleColor();


            display_screen();
            control_window();

            if (bool hovered = ImGui::IsItemHovered()) {               
                camera_controls_in_screen_only();
            }
            camera_controls_outside_screen();

        }

        ImGui::PopStyleColor();
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

    void Screen::control_window() {

        if (!control_open) return; //if it is not enabled, return


        ImGui::SetNextWindowSize(ImVec2(620, 260), ImGuiCond_FirstUseEver);

        // No flags that block move/resize
        ImGui::Begin("Control Panel", &is_enabled);



        if (ImGui::BeginTabBar("Types of Controls", ImGuiTabBarFlags_Reorderable | ImGuiTabBarFlags_FittingPolicyResizeDown)) {

            // -------- Gizmos tab --------
            if (ImGui::BeginTabItem("Transformation (Gizmos)")) {

                static ImGuizmo::OPERATION operation = ImGuizmo::TRANSLATE;
                static ImGuizmo::MODE mode = ImGuizmo::LOCAL;

                
                ImGui::Dummy(ImVec2(0.0f, 20.f));
                ImGui::SeparatorText("Gizmos");
                ImGui::Dummy(ImVec2(0.0f, 20.f));

                ImGui::TextUnformatted("Type: ");
                ImGui::SameLine();
                if (ImGui::RadioButton("Translate", operation == ImGuizmo::TRANSLATE)) operation = ImGuizmo::TRANSLATE;
                ImGui::SameLine();
                if (ImGui::RadioButton("Rotate", operation == ImGuizmo::ROTATE))    operation = ImGuizmo::ROTATE;
                ImGui::SameLine();
                if (ImGui::RadioButton("Scale", operation == ImGuizmo::SCALE))     operation = ImGuizmo::SCALE;
                ImGui::Dummy(ImVec2(0.0f, 20.f));
                
                ImGui::TextUnformatted("Coord Space: ");
                ImGui::SameLine();
                if (ImGui::RadioButton("Local", mode == ImGuizmo::LOCAL))  mode = ImGuizmo::LOCAL;
                ImGui::SameLine();
                if (ImGui::RadioButton("World", mode == ImGuizmo::WORLD))  mode = ImGuizmo::WORLD;
                ImGui::Dummy(ImVec2(0.0f, 200.f));

                

                //if any object is being selected
                if (true) {

                    ImGui::SeparatorText("Object's Transformation");
                    ImGui::Dummy(ImVec2(0.0f, 20.f));
                    ImGui::TextUnformatted("Object Selected (ID): ");
                    ImGui::Dummy(ImVec2(0.0f, 20.f));
                    
                    
                    ImGui::Separator();

                    ImGui::Dummy(ImVec2(0.0f, 20.f));
                    ImGui::TextUnformatted("#Translation: ");
                    ImGui::Dummy(ImVec2(0.0f, 20.f));

      
                    ImGui::Separator();

                    ImGui::Dummy(ImVec2(0.0f, 20.f));
                    ImGui::TextUnformatted("#Scale: ");
                    ImGui::Dummy(ImVec2(0.0f, 20.f));

                    
                    ImGui::Separator();

                    ImGui::Dummy(ImVec2(0.0f, 20.f));
                    ImGui::TextUnformatted("#Rotation: ");
                    ImGui::Dummy(ImVec2(0.0f, 20.f));

                    
                    ImGui::Separator();

                }
                
                ImGui::EndTabItem();
            }

            // -------- Lighting tab --------
            if (ImGui::BeginTabItem("Lighting")) {
                
                ImGui::TextUnformatted("see how");
                

                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Camera Navigation")) {
                ImGui::Spacing();
                ImGui::TextUnformatted("Up Arrrow        or    Q_key   :   Move Up");
                ImGui::Separator();
                ImGui::TextUnformatted("Down Arrrow      or    E_key   :   Move Down");
                ImGui::Separator();
                ImGui::TextUnformatted("Left Arrrow      or    A_key   :   Move Left");
                ImGui::Separator();
                ImGui::TextUnformatted("Right Arrrow     or    D_key   :   Move Right");
                ImGui::Separator();
                ImGui::TextUnformatted("Scoll Backward   or    S_key   :   Move Backwards");
                ImGui::Separator();
                ImGui::TextUnformatted("Scoll Forward    or    W_key   :   Move Forwards");
                ImGui::Separator();
                ImGui::TextUnformatted("Mouse Right Dragged            :   Look around");
                ImGui::EndTabItem();
            }


            ImGui::EndTabBar();
        }

        ImGui::End();

    }

}
#endif
