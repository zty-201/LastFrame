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
    bool Screen::is_control_open = false;
    bool Screen::is_simulator_running = false;
    bool Screen::gizmos_IS_hovering = false;
    bool Screen::gizmos_IS_dragged = false;

    //set these to default first
    static ImGuizmo::OPERATION operation = ImGuizmo::TRANSLATE;
    static ImGuizmo::MODE mode = ImGuizmo::WORLD;
    static ImVec2 available_size{};
    static ImVec2 screen_pos{};
    static float camDistance = 8.f;

    static const float identityMatrix[16] =
    { 1.f, 0.f, 0.f, 0.f,
        0.f, 1.f, 0.f, 0.f,
        0.f, 0.f, 1.f, 0.f,
        0.f, 0.f, 0.f, 1.f };

    Screen::Screen() {
        LOGI("Lfeature: Screen CREATED");
    }
    Screen::~Screen() {

    }
    void Screen::Initialize() {

        GraphicsSystem::GetInstance().GetScene().get()->camerasSetType(WALKING);
    }
    void Screen::Run() {

        GraphicsSystem::GetInstance().GetScene().get()->camerasSetType(WALKING);

        ImGui::PushStyleColor(ImGuiCol_MenuBarBg, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));  // White bar
        ImGui::SetNextWindowSize(ImVec2(800, 800), ImGuiCond_Once);  // Width: 300,500

        if (ImGui::Begin("level loaded: <name>", &is_enabled, ImGuiWindowFlags_MenuBar)) {

            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));      // Black text
            if (ImGui::BeginMenuBar()) {

                if (ImGui::MenuItem("Run Simulator", NULL, &is_simulator_running)) {
                    //std::cout << "Run Simulator\n";

                    int size = (int)GraphicsSystem::GetInstance().GetScene().get()->passes[0].objects.size();

                    //std::cout<< "pass 0 object size: "<<GraphicsSystem::GetInstance().GetScene().get()->passes[0].objects.size() <<std::endl;

                    for (int i = 0; i < size; i++) {

                        //std::cout << "object " << i << " | size = " << GraphicsSystem::GetInstance().GetScene().get()->passes[0].objects[i].instances.size() << std::endl;

                    }

                }

                ImGui::SameLine(0, 50.0f); // The second argument controls the spacing in pixels


                if (ImGui::MenuItem("Interface Controls", NULL, &is_control_open)) {
                    //std::cout << "Controls\n";                   
                }

                ImGui::EndMenuBar();

            }

            ImGui::PopStyleColor();

            display_screen();
            gizmos();
            drop_target();

            if (bool hovered = ImGui::IsItemHovered()) {               
                camera_controls_in_screen_only();
            }
            camera_controls_outside_screen();

        }

        ImGui::PopStyleColor();
        ImGui::End();

        
        
        control_window();
      
    }
    void Screen::Close() {
        LOGI("Screen closed");
    }

    void Screen::display_screen() {

        screen_pos = ImGui::GetCursorScreenPos();
        available_size = ImGui::GetContentRegionAvail();
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

        if (ImGui::IsKeyDown(ImGuiKey_W)) {

            //GraphicsSystem::GetInstance().GetScene().get()->camerasSetPos_andTarget(glm::vec3(0, 0, Mouse_Scroll));
            GraphicsSystem::GetInstance().GetScene().get()->camerasSetPos_andTarget(glm::vec3(0, 0, speed));
        }

        if (ImGui::IsKeyDown(ImGuiKey_S)) {

            GraphicsSystem::GetInstance().GetScene().get()->camerasSetPos_andTarget(glm::vec3(0, 0, -speed));
        }

        if (ImGui::IsKeyDown(ImGuiKey_Q) || ImGui::IsKeyDown(ImGuiKey_UpArrow)) {
            //up vector
            GraphicsSystem::GetInstance().GetScene().get()->camerasSetPos_andTarget(glm::vec3(0, speed, 0));
            //std::cout << "UP !" << std::endl;
        }
        if (ImGui::IsKeyDown(ImGuiKey_E) || ImGui::IsKeyDown(ImGuiKey_DownArrow)) {
            //down vector
            //std::cout << "DOWN !" << std::endl;
            GraphicsSystem::GetInstance().GetScene().get()->camerasSetPos_andTarget(glm::vec3(0, -speed, 0));
        }
        if (ImGui::IsKeyDown(ImGuiKey_A) || ImGui::IsKeyDown(ImGuiKey_LeftArrow)) {
            //left vector
            //std::cout << "LEFT !" << std::endl;
            GraphicsSystem::GetInstance().GetScene().get()->camerasSetPos_andTarget(glm::vec3(-speed, 0, 0));
        }
        if (ImGui::IsKeyDown(ImGuiKey_D) || ImGui::IsKeyDown(ImGuiKey_RightArrow)) {
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

        if (!is_control_open) return; //if it is not enabled, return


        ImGui::SetNextWindowSize(ImVec2(620, 260), ImGuiCond_FirstUseEver);

        // No flags that block move/resize
        ImGui::Begin("Control Panel", &is_control_open);



        if (ImGui::BeginTabBar("Types of Controls", ImGuiTabBarFlags_Reorderable | ImGuiTabBarFlags_FittingPolicyResizeDown)) {

            // -------- Gizmos tab --------
            if (ImGui::BeginTabItem("Transformation (Gizmos)")) {

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

                    static bool transformation_changed = false;

                    ImGui::SeparatorText("Object's Transformation");
                    ImGui::Dummy(ImVec2(0.0f, 20.f));
                    ImGui::TextUnformatted("Object Selected (ID): ");
                    ImGui::Dummy(ImVec2(0.0f, 20.f));

                    auto* scene = GraphicsSystem::GetInstance().GetScene().get();
                    if (!scene) return;
                    glm::mat4& model = scene->passes[0].objects[2].instances[0].modeltransform;

                    float T[3], R[3], S[3];
                    ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(model), T, R, S);
                  
                    ImGui::Separator();

                    ImGui::Dummy(ImVec2(0.0f, 20.f));
                    //ImGui::TextUnformatted("#Translation: ");
                    transformation_changed |= ImGui::InputFloat3("Translation", T, "%.3f");
                    ImGui::Dummy(ImVec2(0.0f, 20.f));

      
                    ImGui::Separator();

                    ImGui::Dummy(ImVec2(0.0f, 20.f));
                    //ImGui::TextUnformatted("#Scale: ");
                    transformation_changed |= ImGui::InputFloat3("Scale", S, "%.3f");
                    
                    
                    ImGui::Dummy(ImVec2(0.0f, 20.f));

                    
                    ImGui::Separator();

                    ImGui::Dummy(ImVec2(0.0f, 20.f));
                    //ImGui::TextUnformatted("#Rotation: ");
                    transformation_changed |= ImGui::InputFloat3("Rotation (deg)", R, "%.3f");
                    ImGui::Dummy(ImVec2(0.0f, 20.f));

                    
                    ImGui::Separator();
                    ImGui::Dummy(ImVec2(0.0f, 20.f));
                    
                    if (ImGui::Button("Reset", ImVec2(120, 0))) {                    
                        R[0] = R[1] = R[2] = 0.0f;
                        S[0] = S[1] = S[2] = 1.0f;                        
                        ImGuizmo::RecomposeMatrixFromComponents(T, R, S, glm::value_ptr(model));
                    }
                    ImGui::SameLine();
                    ImGui::Text(" *only affects scale and rotation");


                    if (transformation_changed) {                       
                        //for (int i = 0; i < 3; ++i) if (fabsf(S[i]) < 1e-6f) S[i] = (S[i] >= 0 ? 1e-6f : -1e-6f);                       
                        ImGuizmo::RecomposeMatrixFromComponents(T, R, S, glm::value_ptr(model));
                    }

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

    //return true if theres a difference
    bool Check_Mat4_Diff(const glm::mat4& a, const glm::mat4& b, float epsilon = 1e-5f) {
        
        const float* pa = glm::value_ptr(a);
        const float* pb = glm::value_ptr(b);
        for (int i = 0; i < 16; ++i) {
            float aa = pa[i];
            float bb = pb[i];
            float tol = epsilon * std::max(1.0f, std::max(std::abs(aa), std::abs(bb)));
            if (std::abs(aa - bb) > tol) return true;
        }
        return false;
    }

    void Screen::gizmos() {

        ImGuizmo::BeginFrame(); 
        ImGuizmo::SetDrawlist(ImGui::GetWindowDrawList());
        gizmos_IS_hovering = ImGuizmo::IsOver();
        gizmos_IS_dragged = ImGuizmo::IsUsing();
     
        auto* scene = GraphicsSystem::GetInstance().GetScene().get();
        auto &cam = scene->passes[0].cameras[0];
        glm::mat4 view = cam.getLookAt();
        glm::mat4 proj = cam.getPerspective(scene->aspect);
        glm::mat4 &obj = scene->passes[0].objects[2].instances[0].modeltransform;
        ImGuizmo::SetOrthographic(false); //because we have perspective
        
        //set this for gizmos
        ImGuizmo::SetRect(screen_pos.x, screen_pos.y, available_size.x, available_size.y);

        //imviewgizmo adapted from imguizmo sample demo       
        float viewManipulateRight = screen_pos.x + available_size.x;
        float viewManipulateTop = screen_pos.y;
        glm::mat4 view_before = view;

        ImGuizmo::DrawCubes(glm::value_ptr(view), glm::value_ptr(proj), glm::value_ptr(obj), 1);
        ImGuizmo::ViewManipulate(glm::value_ptr(view), camDistance, ImVec2(viewManipulateRight - 128, viewManipulateTop), ImVec2(128, 128), 0x10101010);
        
        //whenever we clicked on the cube, view will be return with a new value
        //ONLY if its a new value, set new cam and target position (not every frame)      
        if (Check_Mat4_Diff(view, view_before)) {
            //graphics system is always updating the camera view and proj based on camera pos and target pos
            glm::vec3 new_CamPos = glm::vec3(glm::inverse(view)[3]);
            glm::vec3 new_TargetPos = glm::vec3(obj[3]);
            cam.set_Cam_Pos(new_CamPos);
            cam.set_Target_Pos(new_TargetPos);
        }
        
        ImGuizmo::Manipulate(glm::value_ptr(view),glm::value_ptr(proj),operation,mode, glm::value_ptr(obj));

    }

    void Screen::drop_target() {

        if (ImGui::BeginDragDropTarget())
        {
            // Accept path payload (null-terminated string)
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Models"))
            {                
                //const char* path = (const char*)payload->Data;

                //std::cout << "Drop successful: " << path << std::endl;
            }

            ImGui::EndDragDropTarget();
        }


    }

}
#endif
