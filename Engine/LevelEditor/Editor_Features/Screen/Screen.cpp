/*
*****************************************************************/
/*!
\file   Screen.hpp
\author Tan Yee Tong
\par    yeetong.t\@digipen.edu
\date   September 2025
\brief
This file enables the screen buffer of the level editor to run. It also
includes functions that is built in screen such as gizmos
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

    static std::string droppedPath;

    static const float identityMatrix[16] =
    { 1.f, 0.f, 0.f, 0.f,
        0.f, 1.f, 0.f, 0.f,
        0.f, 0.f, 1.f, 0.f,
        0.f, 0.f, 0.f, 1.f };

    Screen::Screen() {
        this_instance = this;
        LOGI("Lfeature: Screen CREATED");
    }
    Screen::~Screen() {

    }

    Screen& Screen::GetInstance()
    {
        return *this_instance;
    }

    void Screen::Initialize() {
        auto camera_view = registry.view<CameraComponent>();
        auto& camera = camera_view.get<CameraComponent>(camera_view.front());
        Camera::setType(camera, CameraComponent::WALKING);
    }
    void Screen::Run() {

        if (this_instance->is_enabled)
            LOGD("Screen Opened");

        auto camera_view = registry.view<CameraComponent>();
        auto& camera = camera_view.get<CameraComponent>(camera_view.front());
        Camera::setType(camera, CameraComponent::WALKING);

        ImGui::PushStyleColor(ImGuiCol_MenuBarBg, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));  // White bar
        ImGui::SetNextWindowSize(ImVec2(800, 800), ImGuiCond_Once);  // Width: 300,500

        if (ImGui::Begin("level loaded: <name>", NULL, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoCollapse)) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));      // Black text
            if (ImGui::BeginMenuBar()) {

                if (ImGui::MenuItem("Run Simulator", NULL, &is_simulator_running)) {
                    //std::cout << "Run Simulator\n";
                    auto entity_view = registry.view<SceneRenderPass>();
                    int size = (int)entity_view.size();

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
            auto camera_view = registry.view<CameraComponent>();
            auto& camera = camera_view.get<CameraComponent>(camera_view.front());
            Camera::Move_Cam_and_Target(camera, glm::vec3(0, 0, speed));
        }

        if (ImGui::IsKeyDown(ImGuiKey_S)) {
            auto camera_view = registry.view<CameraComponent>();
            auto& camera = camera_view.get<CameraComponent>(camera_view.front());
            Camera::Move_Cam_and_Target(camera, glm::vec3(0, 0, -speed));
        }

        if (ImGui::IsKeyDown(ImGuiKey_Q) || ImGui::IsKeyDown(ImGuiKey_UpArrow)) {
            auto camera_view = registry.view<CameraComponent>();
            auto& camera = camera_view.get<CameraComponent>(camera_view.front());
            Camera::Move_Cam_and_Target(camera, glm::vec3(0, speed, 0));
        }
        if (ImGui::IsKeyDown(ImGuiKey_E) || ImGui::IsKeyDown(ImGuiKey_DownArrow)) {
            auto camera_view = registry.view<CameraComponent>();
            auto& camera = camera_view.get<CameraComponent>(camera_view.front());
            Camera::Move_Cam_and_Target(camera, glm::vec3(0, -speed, 0));
        }
        if (ImGui::IsKeyDown(ImGuiKey_A) || ImGui::IsKeyDown(ImGuiKey_LeftArrow)) {
            auto camera_view = registry.view<CameraComponent>();
            auto& camera = camera_view.get<CameraComponent>(camera_view.front());
            Camera::Move_Cam_and_Target(camera, glm::vec3(-speed, 0, 0));
        }
        if (ImGui::IsKeyDown(ImGuiKey_D) || ImGui::IsKeyDown(ImGuiKey_RightArrow)) {
            auto camera_view = registry.view<CameraComponent>();
            auto& camera = camera_view.get<CameraComponent>(camera_view.front());
            Camera::Move_Cam_and_Target(camera, glm::vec3(speed, 0, 0));
        }

    }
    void Screen::camera_controls_in_screen_only() {

        ImGuiIO& io = ImGui::GetIO();
        ImVec2 Mouse_delta = io.MouseDelta;
        float Mouse_Scroll = io.MouseWheel;

        //right mouse is down means = i am dragging the screen
        if (io.MouseDown[1]) {
            auto camera_view = registry.view<CameraComponent>();
            auto& camera = camera_view.get<CameraComponent>(camera_view.front());
            Camera::onCursor(camera, Mouse_delta.x, Mouse_delta.y);
        }

        //moving forward/backwards
        if (Mouse_Scroll != 0.f) {
            auto camera_view = registry.view<CameraComponent>();
            auto& camera = camera_view.get<CameraComponent>(camera_view.front());
            Camera::Move_Cam_and_Target(camera, glm::vec3(0, 0, Mouse_Scroll));
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
                if (true) { // if any object is being selected
                    static bool transformation_changed = false;

                    ImGui::SeparatorText("Object's Transformation");
                    ImGui::Dummy(ImVec2(0.0f, 20.f));
                    ImGui::TextUnformatted("Object Selected (ID): ");
                    ImGui::Dummy(ImVec2(0.0f, 20.f));

                    auto* scene = GraphicsSystem::GetInstance().GetScene().get();
                    if (!scene) return;
                    auto objects = registry.view<TransformComponent>();
                    auto& first_obj = objects.get<TransformComponent>(objects.front());

                    // Work directly with the component values instead of the matrix
                    float T[3] = { first_obj.position.x, first_obj.position.y, first_obj.position.z };
                    float R[3] = { first_obj.rotation.x, first_obj.rotation.y, first_obj.rotation.z };
                    float S[3] = { first_obj.scale.x, first_obj.scale.y, first_obj.scale.z };

                    ImGui::Separator();

                    ImGui::Dummy(ImVec2(0.0f, 20.f));
                    transformation_changed |= ImGui::InputFloat3("Translation", T, "%.3f");
                    ImGui::Dummy(ImVec2(0.0f, 20.f));

                    ImGui::Separator();

                    ImGui::Dummy(ImVec2(0.0f, 20.f));
                    transformation_changed |= ImGui::InputFloat3("Scale", S, "%.3f");

                    ImGui::Dummy(ImVec2(0.0f, 20.f));

                    ImGui::Separator();

                    ImGui::Dummy(ImVec2(0.0f, 20.f));
                    transformation_changed |= ImGui::InputFloat3("Rotation (deg)", R, "%.3f");
                    ImGui::Dummy(ImVec2(0.0f, 20.f));

                    ImGui::Separator();
                    ImGui::Dummy(ImVec2(0.0f, 20.f));

                    if (ImGui::Button("Reset", ImVec2(120, 0))) {
                        R[0] = R[1] = R[2] = 0.0f;
                        S[0] = S[1] = S[2] = 1.0f;
                        transformation_changed = true;
                    }
                    ImGui::SameLine();
                    ImGui::Text(" *only affects scale and rotation");

                    // Apply changes directly to the transform component
                    if (transformation_changed) {
                        first_obj.position = glm::vec3(T[0], T[1], T[2]);
                        first_obj.rotation = glm::vec3(R[0], R[1], R[2]);
                        first_obj.scale = glm::vec3(S[0], S[1], S[2]);

                        // Reset the flag after applying changes
                        transformation_changed = false;
                    }
                }

                ImGui::EndTabItem();
            }

            // -------- Texture tab --------
            if (ImGui::BeginTabItem("Texture")) {

                if (editor_ptr->selected_objectID == entt::null) {
                    
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));      // Black text
                    ImGui::TextUnformatted("* no object is being selected");
                    ImGui::PopStyleColor();

                }
                else {

                    /*
                       get the material component of the object
                   */
                    ImGui::Dummy(ImVec2(0.0f, 20.f));
                    ImGui::SeparatorText("Material");
                    ImGui::Dummy(ImVec2(0.0f, 20.f));
                    ImGui::TextUnformatted("Assign a material to the object: ");

                    entt::entity id = static_cast<entt::entity>(editor_ptr->selected_objectID);

                    if (registry.all_of<MaterialComponent>(id)) {

                        auto& object = registry.get<MaterialComponent>(id);

                        ImGui::SameLine();
                        static const char* material_type[] = { "Checkerboard", "PBR", "Discard", "Cartoon" };
                        static int index = 0; // index into items

                        if (object == MATERIAL_CHECKERBOARD) {
                            index = 0;
                        }
                        else if (object == MATERIAL_PBR_PHONG) {
                            index = 1;
                        }
                        else if (object == MATERIAL_DISCARD) {
                            index = 2;
                        }
                        else if (object == MATERIAL_CARTOON) {
                            index = 3;
                        }

                        ImGui::SetNextItemWidth(300.f);
                        if (ImGui::Combo("##material_type", &index, material_type, IM_ARRAYSIZE(material_type))) {

                            if (index == 0) {
                                object = MATERIAL_CHECKERBOARD;
                            }
                            else if (index == 1) {
                                object = MATERIAL_PBR_PHONG;
                            }
                            else if (index == 2) {
                                object = MATERIAL_DISCARD;
                            }
                            else if (index == 3) {
                                object = MATERIAL_CARTOON;
                            }

                        }

                    }

                    /*
                       get the texture component of the object
                   */

                    ImGui::Dummy(ImVec2(0.0f, 20.f));
                    ImGui::SeparatorText("Custom Texture");
                    ImGui::Dummy(ImVec2(0.0f, 20.f));

                    ImGui::TextUnformatted("Texture Loaded: ");

                    
                    if (registry.all_of<TextureComponent>(id)) {

                        auto& object = registry.get<TextureComponent>(id);
                        std::string name = AssetManagerSystem::GetInstance().GetName(object.texture_guid.m_Instance);             
                        ImGui::SameLine();
                        
                        ImGui::TextUnformatted(name.c_str());

                        if (ImGui::Button("Remove Custom Texture", ImVec2(300, 0))) {
                            registry.remove<TextureComponent>(id);
                            if (registry.all_of<MaterialComponent>(id)) {

                                auto& object_M = registry.get<MaterialComponent>(id);
                                object_M = MATERIAL_PBR_PHONG;
                            }
                        }

                    }
                    else {
                        ImGui::SameLine();
                        ImGui::TextUnformatted(" Not chosen");
                    }

                    ImGui::Dummy(ImVec2(0.0f, 20.f));
                    
                    ImVec2 size(ImGui::GetContentRegionAvail().x, 160.0f);
                    ImGui::Button("Drop new Texture from Asset Browser to attach to object", size);


                    if (ImGui::BeginDragDropTarget())
                    {
                        // Accept path payload (null-terminated string)
                        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Textures"))
                        {
                            droppedPath = (const char*)payload->Data;

                            //only returning the filename, remove all dir and extension (for asset manager)
                            std::filesystem::path p = std::filesystem::path(droppedPath).lexically_normal();
                            droppedPath = p.stem().string();
                            
                            //std::cout << droppedPath << std::endl;

                            if (registry.all_of<TextureComponent>(id)) {

                                /* if the texture already exists, just swap texture*/
                                auto& object = registry.get<TextureComponent>(id);
                                object.texture_guid = AssetManagerSystem::GetFullGuid(AssetManagerSystem::TEXTURE_GUID, droppedPath);

                            }
                            else {                             
                                /* if the texture do not exists, just init as texture component*/
                                registry.emplace<TextureComponent>(id, AssetManagerSystem::GetFullGuid(AssetManagerSystem::TEXTURE_GUID, droppedPath));
                                
                            }

                        }
                        ImGui::EndDragDropTarget();
                    }

                }


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

        //do not use gizmos if nothing is being selected, because what are you referrring to?
        if (editor_ptr->selected_objectID == entt::null) {
            //std::cout << "nothing is selected\n";
            return;
        }

        auto camera_view = registry.view<CameraComponent>();
        auto& camera = camera_view.get<CameraComponent>(camera_view.front());
        float aspect = (GraphicsSystem::GetInstance().GetWindowSize().second > 0) ?
            (float)GraphicsSystem::GetInstance().GetWindowSize().first / GraphicsSystem::GetInstance().GetWindowSize().second : 1.0f;
        glm::mat4 view = Camera::getLookAt(camera);
        glm::mat4 proj = Camera::getPerspective(camera, aspect);

        /*
            only usw gizmos if the object have a TransformComponent
        */

        entt::entity id = static_cast<entt::entity>(editor_ptr->selected_objectID);
        if (registry.all_of<TransformComponent>(id)) {

            auto& object = registry.get<TransformComponent>(id);

            ImGuizmo::BeginFrame();
            ImGuizmo::SetDrawlist(ImGui::GetWindowDrawList());
            gizmos_IS_hovering = ImGuizmo::IsOver();
            gizmos_IS_dragged = ImGuizmo::IsUsing();



            //auto& first_obj = objects.get<TransformComponent>(objects.front());

            // Create a copy of the model matrix to work with
            glm::mat4 model = object.modelMatrix;
            ImGuizmo::SetOrthographic(false); //because we have perspective

            //set this for gizmos
            ImGuizmo::SetRect(screen_pos.x, screen_pos.y, available_size.x, available_size.y);

            //imviewgizmo adapted from imguizmo sample demo       
            float viewManipulateRight = screen_pos.x + available_size.x;
            float viewManipulateTop = screen_pos.y;
            glm::mat4 view_before = view;

            ImGuizmo::DrawCubes(glm::value_ptr(view), glm::value_ptr(proj), glm::value_ptr(model), 1);
            ImGuizmo::ViewManipulate(glm::value_ptr(view), camDistance, ImVec2(viewManipulateRight - 128, viewManipulateTop), ImVec2(128, 128), 0x10101010);

            //whenever we clicked on the cube, view will be return with a new value
            //ONLY if its a new value, set new cam and target position (not every frame)      
            if (Check_Mat4_Diff(view, view_before)) {
                //graphics system is always updating the camera view and proj based on camera pos and target pos
                glm::vec3 new_CamPos = glm::vec3(glm::inverse(view)[3]);
                glm::vec3 new_TargetPos = glm::vec3(model[3]);
                Camera::set_Cam_Pos(camera, new_CamPos);
                Camera::set_Target_Pos(camera, new_TargetPos);
            }

            // Store the matrix before manipulation
            glm::mat4 model_before = model;

            // Manipulate the model matrix
            ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(proj), operation, mode, glm::value_ptr(model));

            // Check if the model matrix was changed
            if (Check_Mat4_Diff(model, model_before)) {
                // Extract position, rotation, and scale from the new model matrix
                float translation[3], rotation[3], scale[3];
                ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(model), translation, rotation, scale);

                // Update the TransformComponent values
                object.position = glm::vec3(translation[0], translation[1], translation[2]);
                object.rotation = glm::vec3(rotation[0], rotation[1], rotation[2]);
                object.scale = glm::vec3(scale[0], scale[1], scale[2]);

                // No need to manually update the modelMatrix as it will be recalculated next frame
                // based on the updated position, rotation, and scale values
            }


        }


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
