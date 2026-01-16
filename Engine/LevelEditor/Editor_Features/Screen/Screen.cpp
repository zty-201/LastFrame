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

//GLM
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>

//File management, JSON file parser/writer
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

//Entt reflection system
#include <entt/meta/meta.hpp>
#include <entt/meta/resolve.hpp>
//Entt ECS system
#include <entt/entity/registry.hpp>
#include <entt/entt.hpp>
//#include <entt/entity/snapshot.hpp>
#include "System_LevelEditor.hpp"

//Casting meta data to type.
#include "ReflectionRegister.hpp"
#include "GraphicsSystem.h"
#include "Screen.hpp"
#include "imgui.h"
#include "ImGuizmo.h"
#include "TriggerBoxComponent.h"
#include "MeshComponent.h"
#include "ColliderComponent.h"
#include "TriggerSystem.hpp"
#include "TriggerEvents.hpp"
#include "Editor_Features/CameraController/EditorCameraController.hpp"

//#include <iostream>

//Scripting related
#include "Script.hpp"
#include "Scripting/ScriptAsset.hpp"
#include "AssetManager.hpp"

//Camera x scripting
#include "CameraComponent.h"
#include "Camera.h"

//Creation of instances
#include "System_GameObjectSystem.hpp"
#include "SceneManager.hpp"
#include "PhysicsWorld.hpp"

namespace EngineFramework
{
    using namespace entt::literals;

    bool Screen::is_control_open = false;
    bool Screen::is_simulator_running = false;
    bool Screen::gizmos_IS_hovering = false;
    bool Screen::gizmos_IS_dragged = false;
    bool Screen::is_adjusting_lights = false;
    bool Screen::is_adjusting_triggerbox = false;
    bool Screen::s_DrawColliders = true;
    bool Screen::s_DrawAllColliders = false;
    
    //
    std::string Screen::json_backup{};

    bool gizmos_started_drag = false;
    std::unique_ptr<Record> current_record = nullptr;
    
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

        m_snapEnabled = false;
        m_snapStep = 1.0f;

    }
    Screen::~Screen() {

        current_record.reset();
    }

    Screen& Screen::GetInstance()
    {
        return *this_instance;
    }

    inline int GetDominantScaleAxis(const float oldScale[3], const float newScale[3])
    {
        float dx = fabs(newScale[0] - oldScale[0]);
        float dy = fabs(newScale[1] - oldScale[1]);
        float dz = fabs(newScale[2] - oldScale[2]);

        if (dx >= dy && dx >= dz) return 0;
        if (dy >= dx && dy >= dz) return 1;
        return 2;
    }

    static inline glm::mat4 TRS(const glm::vec3& t, const glm::quat& q, const glm::vec3& s) {
        glm::mat4 M(1.0f);
        M = glm::translate(M, t);
        M *= glm::mat4_cast(q);
        M = glm::scale(M, s);
        return M;
    }

    static inline glm::quat EulerDegToQuat(const glm::vec3& deg) {
        return glm::quat(glm::radians(deg));
    }

    static void FitTriggerToMesh(entt::registry& reg, entt::entity id)
    {
        LOGI("[TriggerBox] Fitting entity %u", (unsigned)id);

        if (!reg.all_of<MeshComponent, TriggerBoxComponent>(id))
            return;

        auto& meshComp = reg.get<MeshComponent>(id);
        auto& box = reg.get<TriggerBoxComponent>(id);

        LOGI("[TriggerBox] Fitting for entity %u, meshName='%s'",
            (unsigned)id, meshComp.meshName.c_str());

        // 1) Get guid from mesh name
        xresource::full_guid meshGuid =
            EngineFramework::AssetManagerSystem::GetFullGuid(
                EngineFramework::AssetManagerSystem::MESH_GUID,
                meshComp.meshName);

        if (!EngineFramework::AssetManagerSystem::HasResource(meshGuid))
        {
            LOGI("[TriggerBox] Mesh resource NOT found for '%s'; using default trigger size.",
                meshComp.meshName.c_str());
            return;
        }

        // 2) Get the actual MeshData
        auto meshData =
            EngineFramework::AssetManagerSystem::GetResource<Mesh::MeshData>(meshGuid);
        if (!meshData)
        {
            LOGI("[TriggerBox] MeshData is null for '%s'; using default trigger size.",
                meshComp.meshName.c_str());
            return;
        }

        // 3) Use MeshData's AABB (from the OBJ vertices)
        const auto& aabb = meshData->GetAABB();
        glm::vec3 center = aabb.center();
        glm::vec3 half = aabb.halfExtents();

        LOGI("[TriggerBox] mesh='%s' center(%.3f, %.3f, %.3f) half(%.3f, %.3f, %.3f)",
            meshComp.meshName.c_str(),
            center.x, center.y, center.z,
            half.x, half.y, half.z);

        box.type = TriggerShapeType::Box;
        box.localPosition = center;
        box.halfExtents = half;
    }

    static inline bool ProjectToScreen(const glm::mat4& V, const glm::mat4& P, const glm::vec3& p,
        ImVec2 pos, ImVec2 size, ImVec2& out) {
        glm::vec4 clip = P * V * glm::vec4(p, 1.0f);
        if (clip.w == 0.0f) return false;
        glm::vec3 ndc = glm::vec3(clip) / clip.w;
        if (ndc.z < -1.0f || ndc.z > 1.0f) return false;
        out.x = (ndc.x * 0.5f + 0.5f) * size.x + pos.x;
        out.y = (ndc.y * -0.5f + 0.5f) * size.y + pos.y;
        return true;
    }

    static void DrawWireOBB(const glm::mat4& M, const glm::mat4& V, const glm::mat4& P,
        ImVec2 pos, ImVec2 size, ImU32 color = IM_COL32(0, 255, 0, 255), float thick = 1.6f) {
        static const glm::vec3 C[8] = { {-0.5,-0.5,-0.5},{0.5,-0.5,-0.5},{0.5,0.5,-0.5},{-0.5,0.5,-0.5},
                                     {-0.5,-0.5,0.5},{0.5,-0.5,0.5},{0.5,0.5,0.5},{-0.5,0.5,0.5} };
        static const int E[12][2] = { {0,1},{1,2},{2,3},{3,0},{4,5},{5,6},{6,7},{7,4},{0,4},{1,5},{2,6},{3,7} };
        ImDrawList* dl = ImGui::GetWindowDrawList(); ImVec2 pts[8]; bool ok[8];
        for (int i = 0; i < 8; ++i) { glm::vec3 w = glm::vec3(M * glm::vec4(C[i], 1)); ok[i] = ProjectToScreen(V, P, w, pos, size, pts[i]); }
        for (auto& e : E) if (ok[e[0]] && ok[e[1]]) dl->AddLine(pts[e[0]], pts[e[1]], color, thick);
    }

    // To ensure all entities selected (not just entities with collider component) will get highlighted
    static bool BuildSelectionOBB(entt::entity e, glm::mat4& out_matrix) {
        if (!registry.all_of<TransformComponent>(e)) {
            return false;
        }

        const auto& tf = registry.get<TransformComponent>(e);
        glm::vec3 center(0.0f);
        glm::vec3 half(0.5f);

        if (registry.all_of<MeshComponent>(e)) {
            const auto& mesh = registry.get<MeshComponent>(e);
            glm::vec3 min(mesh.localMin[0], mesh.localMin[1], mesh.localMin[2]);
            glm::vec3 max(mesh.localMax[0], mesh.localMax[1], mesh.localMax[2]);
            center = (min + max) * 0.5f;
            half = (max - min) * 0.5f;
        }

        glm::mat4 local = glm::translate(glm::mat4(1.0f), center) * glm::scale(glm::mat4(1.0f), half * 2.0f);
        out_matrix = tf.modelMatrix * local;
        return true;
    }

    static void OnTriggerEnter(const Event_OnTriggerEnter& e)
    {
        LOGI("[TriggerEnter] Trigger %u -> Entity %u",
            (unsigned)e.trigger, (unsigned)e.other);
    }

    static void OnTriggerStay(const Event_OnTriggerStay& e)
    {
        LOGI("[TriggerStay] Trigger %u -> Entity %u",
            (unsigned)e.trigger, (unsigned)e.other);
    }

    static void OnTriggerExit(const Event_OnTriggerExit& e)
    {
        LOGI("[TriggerExit] Trigger %u -> Entity %u",
            (unsigned)e.trigger, (unsigned)e.other);
    }

    void Screen::Initialize() {
        LOGI("[Screen] Initializing main camera...");

        auto camera_view = registry.view<CameraComponent>();
        if (camera_view.empty()) {
            LOGE("[Screen] No CameraComponent found! Cannot attach Lua camera script.");
            return;
        }
        else
            LOGI("[Screen] CameraComponent found in registry, proceeding to attach Lua script.");

        auto cameraEntity = camera_view.front();
        auto& camera = camera_view.get<CameraComponent>(cameraEntity);
        Camera::setType(camera, CameraComponent::WALKING);
        history = std::make_unique<HistorySystem>(5);
#if defined(PLATFORM_ANDROID) 
		is_simulator_running = true;
#endif
#if defined(PLATFORM_WINDOWS)
		is_simulator_running = false;
#endif
        camera_speed = 0.5f; // as default
        scrolling_speed = 10.f; // default

        Event_Level_Editor_Simulation editor_toggled{};
        editor_toggled.is_enabled = false;
        event_dispatcher.trigger<Event_Level_Editor_Simulation>(std::move(editor_toggled));
        event_dispatcher.sink<Event_OnTriggerEnter>().connect<&OnTriggerEnter>();
        event_dispatcher.sink<Event_OnTriggerStay>().connect<&OnTriggerStay>();
        event_dispatcher.sink<Event_OnTriggerExit>().connect<&OnTriggerExit>();

		//No longer auto attach Lua camera script here
//#if defined(ENGINE_ENABLE_LUA)
//        // Attach Lua orbit/drag camera control script
//        LOGI("[Screen] Attaching Lua script 'camera_touch_control' to camera entity...");
//
//        // Check if it already has a LuaScriptComponent
//        if (!registry.all_of<Engine::LuaScriptComponent>(cameraEntity)) {
//            registry.emplace<Engine::LuaScriptComponent>(cameraEntity);
//        }
//
//        auto& lsc = registry.get<Engine::LuaScriptComponent>(cameraEntity);
//        lsc.Enabled = true;
//
//        // Build a LuaInstance for the camera script
//        Engine::LuaScriptComponent::LuaInstance inst;
//        inst.ScriptGuid = EngineFramework::AssetManagerSystem::GetFullGuid(
//            EngineFramework::AssetManagerSystem::SCRIPT_GUID,
//            "camera_touch_control"
//        ).m_Instance;
//
//        // Avoid duplicates (in case of reload)
//        bool alreadyAttached = false;
//        for (auto& s : lsc.Scripts) {
//            if (s.ScriptGuid == inst.ScriptGuid) {
//                alreadyAttached = true;
//                LOGE("[Screen] camera_touch_control already attached to camera, skipping reattach.");
//                break;
//            }
//        }
//
//        if (!alreadyAttached) {
//            lsc.Scripts.push_back(inst);
//            LOGI("[Screen] camera_touch_control successfully attached to camera entity (%u).",
//                static_cast<unsigned>(cameraEntity));
//
//            // Load and run immediately
//            Engine::ScriptSystem::ReloadScript(registry, cameraEntity);
//        }
//#endif
    }

    void Screen::Run() {

        /* if (this_instance->is_enabled)
             LOGD("Screen Opened");*/

        auto camera_view = registry.view<CameraComponent>();
        //auto& camera = camera_view.get<CameraComponent>(camera_view.front());
        //Camera::setType(camera, CameraComponent::ORBITING);

        ImGui::PushStyleColor(ImGuiCol_MenuBarBg, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));  // White bar
        ImGui::SetNextWindowSize(ImVec2(800, 800), ImGuiCond_Once);  // Width: 300,500

        std::string levelLoaded = "Level Loaded: " + SceneManager::GetInstance().GetCurrentScene();

        if (ImGui::Begin(levelLoaded.c_str(), NULL, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoCollapse)) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));      // Black text
            if (ImGui::BeginMenuBar()) {

                static std::string caption = "Run Simulator";
                if (is_simulator_running) {
                    caption = "Stop Simulator";
                }
                else {
                    caption = "Run Simulator";
                }

                if (ImGui::MenuItem(caption.c_str())) {

                    if (!is_simulator_running) play_simulation();
                    else stop_simulation();

                }

                if (!is_simulator_running) {

                    ImGui::SameLine(0, 50.0f); // The second argument controls the spacing in pixels


                    if (ImGui::MenuItem("Interface Controls", NULL, &is_control_open)) {
                        //std::cout << "Controls\n";                   
                    }

                    ImGui::SameLine(0, 50.0f); // The second argument controls the spacing in pixels


                    if (ImGui::MenuItem("Undo")) {
                        //std::cout << "undo is clicked\n";
                        history->undo();
                    }

                    float requiredWidth = 250.f;
                    if (s_DrawColliders)
                    {
                        requiredWidth += 200.f;
                    }
                    float menuBarWidth = ImGui::GetContentRegionAvail().x;
                    if (menuBarWidth > requiredWidth)
                    {
                        ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - requiredWidth);
                    }
                    bool debugPhysics = s_DrawColliders;
                    if (ImGui::Checkbox("Debug Collider", &debugPhysics))
                    {
                        Screen::s_DrawColliders = debugPhysics;
                    }
                    if (debugPhysics)
                    {
                        ImGui::SameLine();
                        bool showAll = Screen::s_DrawAllColliders;
                        if (ImGui::Checkbox("Show All", &showAll))
                        {
                            Screen::s_DrawAllColliders = showAll;
                        }
                    }
                }
                ImGui::EndMenuBar();
            }

            ImGui::PopStyleColor();

            if (SceneManager::GetInstance().is_file_loaded) {

                display_screen();
                bool hovered = ImGui::IsItemHovered();

                gizmos();
                //triggerbox();
                drop_target();
                colliderbox();

                EngineFramework::TriggerSystem::Update(registry);
                auto tfView = registry.view<TransformComponent>();
                for (auto ent : tfView)
                {
                    auto& tf = tfView.get<TransformComponent>(ent);
                    tf.prevModelMatrix = tf.modelMatrix;
                }

                if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && hovered && !ImGuizmo::IsOver())
                {
                    ImVec2 mousePos = ImGui::GetMousePos();
                    ImVec2 viewportPos = ImGui::GetItemRectMin();
                    ImVec2 viewportSize = ImGui::GetItemRectSize();

                    float localX = mousePos.x - viewportPos.x;
                    float localY = mousePos.y - viewportPos.y;

                    if (!camera_view.empty())
                    {
                        const auto& cam = registry.get<CameraComponent>(camera_view.front());

                        glm::vec3 rayOrigin, rayDir;
                        Camera::ScreenPointToRay(
                            cam,
                            localX, localY,
                            viewportSize.x, viewportSize.y,
                            rayOrigin, rayDir
                        );

                        RaycastHit hit;
                        if (PhysicsWorld::Get().Raycast(rayOrigin, rayDir, 1000.f, hit))
                        {
                            LOGI("Mouse Picked Entity: %u", (uint32_t)hit.entity);
                            LevelEditor::GetInstance().variables.selected_objectID = hit.entity;
                            LevelEditor::GetInstance().variables.latest_selected = iEditorView::Latest_Selected::INSTANCE_SELECTED;
                        }
                        else
                        {
                            //Deselect if clicking empty space
                            LevelEditor::GetInstance().variables.selected_objectID = entt::null;
                            LevelEditor::GetInstance().variables.latest_selected = iEditorView::Latest_Selected::NONE;

                            LOGI("Selection cleared.");
                        }
                    }
                }

                if (hovered) {
                    camera_controls_in_screen_only();
                }
                camera_controls_outside_screen();

                static bool right_clicked = false;
                static ImVec2 pressPos;
                ImGuiIO& io = ImGui::GetIO();
                if (ImGui::IsMouseClicked(ImGuiMouseButton_Right) && hovered)
                {
                    right_clicked = true;
                    pressPos = io.MousePos;

                }

                //open pop up if the clicked is released
                if (ImGui::IsMouseReleased(ImGuiMouseButton_Right) && right_clicked)
                {
                    float dx = io.MousePos.x - pressPos.x;
                    float dy = io.MousePos.y - pressPos.y;
                    float dist2 = dx * dx + dy * dy;

                    // threshold to ignore orbiting / dragging
                    const float CLICK_EPSILON = 2.0f;

                    // only open menu if mouse barely moved
                    if (dist2 < CLICK_EPSILON * CLICK_EPSILON)
                    {
                        ImGui::OpenPopup("Entity_Shortcut_Menu");
                    }
                }
                entity_shortcut_menu();
                adjust_triggerbox();

                if (!io.WantTextInput) {
                    const ImGuiInputFlags delete_shortcut_flags = ImGuiInputFlags_RouteGlobal | ImGuiInputFlags_RouteOverFocused;
                    if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_Delete, delete_shortcut_flags)) {
                        delete_selected_entity();
                    }
                }
            }
        }
        ImGui::PopStyleColor();
        ImGui::End();
        control_window();
    }
    void Screen::Close() {
        clear_backup();
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

    /*void Screen::camera_controls_outside_screen() {

        if (is_simulator_running) return;

        if (ImGui::IsKeyDown(ImGuiKey_W)) {
            auto camera_view = registry.view<CameraComponent>();
            auto& camera = camera_view.get<CameraComponent>(camera_view.front());
            Camera::Move_Cam_and_Target(camera, glm::vec3(0, 0, camera_speed));
        }

        if (ImGui::IsKeyDown(ImGuiKey_S)) {
            auto camera_view = registry.view<CameraComponent>();
            auto& camera = camera_view.get<CameraComponent>(camera_view.front());
            Camera::Move_Cam_and_Target(camera, glm::vec3(0, 0, -camera_speed));
        }

        if (ImGui::IsKeyDown(ImGuiKey_Q) || ImGui::IsKeyDown(ImGuiKey_UpArrow)) {
            auto camera_view = registry.view<CameraComponent>();
            auto& camera = camera_view.get<CameraComponent>(camera_view.front());
            Camera::Move_Cam_and_Target(camera, glm::vec3(0, camera_speed, 0));
        }
        if (ImGui::IsKeyDown(ImGuiKey_E) || ImGui::IsKeyDown(ImGuiKey_DownArrow)) {
            auto camera_view = registry.view<CameraComponent>();
            auto& camera = camera_view.get<CameraComponent>(camera_view.front());
            Camera::Move_Cam_and_Target(camera, glm::vec3(0, -camera_speed, 0));
        }
        if (ImGui::IsKeyDown(ImGuiKey_A) || ImGui::IsKeyDown(ImGuiKey_LeftArrow)) {
            auto camera_view = registry.view<CameraComponent>();
            auto& camera = camera_view.get<CameraComponent>(camera_view.front());
            Camera::Move_Cam_and_Target(camera, glm::vec3(-camera_speed, 0, 0));
        }
        if (ImGui::IsKeyDown(ImGuiKey_D) || ImGui::IsKeyDown(ImGuiKey_RightArrow)) {
            auto camera_view = registry.view<CameraComponent>();
            auto& camera = camera_view.get<CameraComponent>(camera_view.front());
            Camera::Move_Cam_and_Target(camera, glm::vec3(camera_speed, 0, 0));
        }
    }*/


    /*void Screen::camera_controls_in_screen_only() {

        if (is_simulator_running) return;

        ImGuiIO& io = ImGui::GetIO();
        ImVec2 Mouse_delta = io.MouseDelta;
        float Mouse_Scroll = io.MouseWheel * scrolling_speed;

        if (is_adjusting_lights) {
            
            if (io.MouseDown[0]) {
                auto light_view = registry.view<LightComponent>();
                auto& light = light_view.get<LightComponent>(light_view.front());

                if (Mouse_delta.x > 0.0 || Mouse_delta.y > 0.0) {
                    Light::onCursor(light, Mouse_delta.x, Mouse_delta.y);
                    //std::cout << "Mouse_delta.x: " << Mouse_delta.x << " | Mouse_delta.y: " << Mouse_delta.y << std::endl;
                }
            }
        }

        //right mouse is down means = i am dragging the screen
        if (io.MouseDown[1]) {
            auto camera_view = registry.view<CameraComponent>();
            auto& camera = camera_view.get<CameraComponent>(camera_view.front());
            Camera::onCursor(camera, camera_speed * Mouse_delta.x, camera_speed * Mouse_delta.y);
        }

        //moving forward/backwards
        if (Mouse_Scroll != 0.f) {
            auto camera_view = registry.view<CameraComponent>();
            auto& camera = camera_view.get<CameraComponent>(camera_view.front());
            Camera::Move_Cam_and_Target(camera, glm::vec3(0, 0, Mouse_Scroll));
        }

    }*/

    /* ---------------------------
    *    Updated camera controls
    ------------------------------*/

    void Screen::camera_controls_outside_screen()
    {
        if (is_simulator_running) return;
        
        // Only keyboard movement when mouse is NOT over the viewport.
        EditorCameraInput input{};

        // You can tune this per-frame if you want.
        m_editorCamera.moveSpeed = camera_speed;

        // WASD + arrows
        input.keyW = ImGui::IsKeyDown(ImGuiKey_W);
        input.keyA = ImGui::IsKeyDown(ImGuiKey_A);
        input.keyS = ImGui::IsKeyDown(ImGuiKey_S);
        input.keyD = ImGui::IsKeyDown(ImGuiKey_D);
        input.keyQ = ImGui::IsKeyDown(ImGuiKey_Q) || ImGui::IsKeyDown(ImGuiKey_UpArrow);
        input.keyE = ImGui::IsKeyDown(ImGuiKey_E) || ImGui::IsKeyDown(ImGuiKey_DownArrow);
        input.keyLeft = ImGui::IsKeyDown(ImGuiKey_LeftArrow);
        input.keyRight = ImGui::IsKeyDown(ImGuiKey_RightArrow);

        // Focus / frame selected (e.g. F key)
        input.keyFocus = ImGui::IsKeyPressed(ImGuiKey_F);

        auto camera_view = registry.view<CameraComponent>();
        if (camera_view.empty())
            return;

        entt::entity camEnt = camera_view.front();
        auto& cam = camera_view.get<CameraComponent>(camEnt);

        // We don�t use mouse or focus here (outside viewport).
        entt::entity selected = static_cast<entt::entity>(editor_ptr->selected_objectID);

        m_editorCamera.Update(cam, input, registry, selected);
    }

    void Screen::camera_controls_in_screen_only()
    {
        if (is_simulator_running) return;
       
        ImGuiIO& io = ImGui::GetIO();

        EditorCameraInput input{};

        m_editorCamera.zoomSpeed = scrolling_speed;

        // Mouse buttons
        input.rightMouseDown = io.MouseDown[1];   // RMB = orbit / look
        input.middleMouseDown = io.MouseDown[2];   // MMB = pan

        // Mouse movement & scroll
        input.mouseDeltaX = io.MouseDelta.x;
        input.mouseDeltaY = io.MouseDelta.y;
        input.mouseScroll = io.MouseWheel;

        /*
        * calling twice hence i commented, outside() are called outside and inside, so u do not need this
        */

        //Keyboard while hovering over the viewport
        //input.keyW = ImGui::IsKeyDown(ImGuiKey_W);
        //input.keyA = ImGui::IsKeyDown(ImGuiKey_A);
        //input.keyS = ImGui::IsKeyDown(ImGuiKey_S);
        //input.keyD = ImGui::IsKeyDown(ImGuiKey_D);
        //input.keyQ = ImGui::IsKeyDown(ImGuiKey_Q) || ImGui::IsKeyDown(ImGuiKey_UpArrow);
        //input.keyE = ImGui::IsKeyDown(ImGuiKey_E) || ImGui::IsKeyDown(ImGuiKey_DownArrow);
        //input.keyLeft = ImGui::IsKeyDown(ImGuiKey_LeftArrow);
        //input.keyRight = ImGui::IsKeyDown(ImGuiKey_RightArrow);

        //// Focus / frame selected (e.g. F key)
        //input.keyFocus = ImGui::IsKeyPressed(ImGuiKey_F);

        auto camera_view = registry.view<CameraComponent>();
        if (camera_view.empty())
            return;

        entt::entity camEnt = camera_view.front();
        auto& cam = camera_view.get<CameraComponent>(camEnt);

        entt::entity selected = static_cast<entt::entity>(editor_ptr->selected_objectID);

        m_editorCamera.Update(cam, input, registry, selected);

        /*
            for adjusting the lighting 
        */
        if (is_adjusting_lights) {

            if (io.MouseDown[0]) {
                auto light_view = registry.view<LightComponent>();
                auto& light = light_view.get<LightComponent>(light_view.front());

                if (io.MouseDelta.x > 0.0 || io.MouseDelta.y > 0.0) {
                    Light::onCursor(light, io.MouseDelta.x, io.MouseDelta.y);
                    //std::cout << "Mouse_delta.x: " << Mouse_delta.x << " | Mouse_delta.y: " << Mouse_delta.y << std::endl;
                }

            }
        }

    }

    void Screen::control_window() {

        if (!is_control_open) return; //if it is not enabled, return
        if (is_simulator_running) return;

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

                ImGui::Dummy(ImVec2(0.0f, 10.f));
                ImGui::SeparatorText("Snapping");

                ImGui::Checkbox("Enable Object Snap", &m_snapEnabled);

                ImGui::SameLine();
                ImGui::SetNextItemWidth(200.0f);
                if (ImGui::InputFloat("Step", &m_snapStep, 1.f, 5.0f, "%.3f"))
                {
                    if (m_snapStep <= 0.0f)
                    {
                        m_snapStep = 0.1f;  // Prevent zero/negative value
                    }
                }

                ImGui::Dummy(ImVec2(0.0f, 10.f));

                //only if selected object have transform component
                entt::entity id = static_cast<entt::entity>(editor_ptr->selected_objectID);

                if (registry.all_of<TransformComponent>(id)) {

                    auto& object = registry.get<TransformComponent>(id);

                    float T[3] = { object.position.x, object.position.y, object.position.z };
                    float R[3] = { object.rotation.x, object.rotation.y, object.rotation.z };
                    float S[3] = { object.scale.x, object.scale.y, object.scale.z };

                    static std::unique_ptr<Record> record_pos = nullptr;
                    static std::unique_ptr<Record> record_scale = nullptr;
                    static std::unique_ptr<Record> record_rot = nullptr;

                    if (ImGui::InputFloat3("Translation", T, "%.3f")) {
                        if (!record_pos) {
                            record_pos = history->start_track(id, "TransformComponent", "position");
                        }
                    }
                    if (record_pos && ImGui::IsItemDeactivatedAfterEdit()) {
                        history->end_track(record_pos, object.position);
                        record_pos.reset();
                    }

                    ImGui::Dummy(ImVec2(0.0f, 20.f));

                    if (ImGui::InputFloat3("Scale", S, "%.3f")) {
                        if (!record_scale) {
                            record_scale = history->start_track(id, "TransformComponent", "scale");
                        }
                    }
                    if (record_scale && ImGui::IsItemDeactivatedAfterEdit()) {
                        history->end_track(record_scale, object.scale);
                        record_scale.reset();
                    }

                    ImGui::Dummy(ImVec2(0.0f, 20.f));

                    if (ImGui::InputFloat3("Rotation (deg)", R, "%.3f")) {
                        if (!record_rot) {
                            record_rot = history->start_track(id, "TransformComponent", "rotation");
                        }
                    }
                    if (record_rot && ImGui::IsItemDeactivatedAfterEdit()) {
                        history->end_track(record_rot, object.rotation);
                        record_rot.reset();
                    }

                    object.position = glm::vec3(T[0], T[1], T[2]);
                    object.scale = glm::vec3(S[0], S[1], S[2]);
                    object.rotation = glm::vec3(R[0], R[1], R[2]);

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
                ImGui::TextUnformatted("Scroll Backward  or    S_key   :   Move Backwards");
                ImGui::Separator();
                ImGui::TextUnformatted("Scroll Forward   or    W_key   :   Move Forwards");
                ImGui::Separator();
                ImGui::TextUnformatted("Mouse Right Dragged            :   Look around");

                static float camera_temp = camera_speed;  // current value
                
                ImGui::InputFloat("Camera Movement Speed", &camera_temp);
                if (ImGui::IsItemDeactivatedAfterEdit())
                {
                    
                    if (camera_temp >= 0.0f)
                    {
                        camera_speed = camera_temp;       
                    }
                    
                }

                static float scroll_temp = scrolling_speed;  // current value

                ImGui::InputFloat("Mouse Scroll Speed", &scroll_temp);
                if (ImGui::IsItemDeactivatedAfterEdit())
                {
                    
                    if (scroll_temp >= 0.0f)
                    {
                        scrolling_speed = scroll_temp;      
                    }

                }



                ImGui::EndTabItem();
            }

            // -------- Environment / Lighting tab --------
            if (ImGui::BeginTabItem("Environment")) {
                ImGui::Spacing();

                ImGui::Spacing();
                ImGui::Separator();

                // Fog control parameters
                ImGui::SeparatorText("Fog Control Parameters");
                ImGui::Spacing();

                // Get the graphics system reference
                auto& gfx = GraphicsSystem::GetInstance();

                // Anisotropy
                float aniso = gfx.GetAnisotrophy();
                if (ImGui::InputFloat("Anisotropy", &aniso, 0.f, 0.f, "%.4f")) {
                    gfx.SetAnisotrophy(aniso);
                }

                // Density
                float density = gfx.GetDensity();
                if (ImGui::InputFloat("Density", &density, 0.f, 0.f, "%.4f")) {
                    gfx.SetDensity(density);
                }

                // Scattering
                float scattering = gfx.GetScattering();
                if (ImGui::InputFloat("Scattering", &scattering, 0.f, 0.f, "%.4f")) {
                    gfx.SetScattering(scattering);
                }

                // Absorption
                float absorption = gfx.GetAbsorption();
                if (ImGui::InputFloat("Absorption", &absorption, 0.f, 0.f, "%.4f")) {
                    gfx.SetAbsorption(absorption);
                }

                // Shadow Bias
                float shadow_bias = gfx.GetShadowBias();
                if (ImGui::InputFloat("Shadow Bias", &shadow_bias, 0.f, 0.f, "%.4f")) {
                    gfx.SetShadowBias(shadow_bias);
                }

                // Depth Power
                float depth_power = gfx.GetDepthPower();
                if (ImGui::InputFloat("Depth Power", &depth_power, 0.f, 0.f, "%.4f")) {
                    gfx.SetDepthPower(depth_power);
                }


                // Ambient Light settings
                float ambient_intensity = gfx.GetAmbientLightIntensity();
                if (ImGui::InputFloat("Ambient Light Intensity", &ambient_intensity, 0.f, 0.f, "%.4f")) {
                    gfx.SetAmbientLightIntensity(ambient_intensity);
                }

                // Temporal accumulation
                //bool temporal_accumulation = gfx.GetTemporalAccumulation();
    //            if (ImGui::Checkbox("Temporal Accumulation", &temporal_accumulation)) {
    //                gfx.SetTemporalAccumulation(temporal_accumulation);
    //            }

                //// Tricubic Filtering
                //bool tricubic_filtering = gfx.GetTricubicFiltering();
    //            if (ImGui::Checkbox("Tricubic Filtering", &tricubic_filtering)) {
    //                gfx.SetTricubicFiltering(tricubic_filtering);
                //}

                ImGui::Spacing();
                ImGui::Separator();

                // Start Light settings
                ImGui::SeparatorText("Lights");
                ImGui::Spacing();

                // Gather lights from registry
                std::vector<std::string> lightNames;
                std::vector<entt::entity> lightEntities;
                for (auto entity : registry.view<LightComponent>()) {
                    std::stringstream ss;
                    ss << "Light " << lightNames.size() << " (ent " << static_cast<uint32_t>(entity) << ")";
                    lightNames.push_back(ss.str());
                    lightEntities.push_back(entity);
                }

                static int selected_light_idx = 0;

                // Check for lights and adjust the lights if needed
                if (!lightNames.empty()) {
                    // Create a drop down to select the specific light
                    // First create a char* array for ImGui to read the names
                    std::vector<const char*> lightNamePtrs;
                    lightNamePtrs.reserve(lightNames.size());

                    // Loop through to get the char* pointers
                    for (const auto& name : lightNames) {
                        lightNamePtrs.push_back(name.c_str());
                    }

                    // Check if an overflow (in index) occurs
                    if (selected_light_idx >= static_cast<int>(lightNames.size())) {
                        selected_light_idx = 0;
                    }

                    ImGui::Combo("Select Light", &selected_light_idx,
                        lightNamePtrs.data(), static_cast<int>(lightNamePtrs.size()));

                    // Obtain the selected light entity to adjust values
                    entt::entity selected_light_entity = lightEntities[selected_light_idx];

                    // Check if the entity is valid and has LightComponent
                    if (registry.valid(selected_light_entity) && registry.all_of<LightComponent>(selected_light_entity)) {
                        auto& light = registry.get<LightComponent>(selected_light_entity);

                        // Intensity
                        float intensity = light.intensity;
                        if (ImGui::InputFloat("Light Intensity", &intensity)) {
                            light.intensity = intensity;
                        }

                        // Controls specific to light type
                        if (light.type == LightComponent::POINT) { // Point Light
                            float pos[3] = { light.position.x, light.position.y, light.position.z };
                            if (ImGui::InputFloat3("Light Position", pos, "%.3f")) {
                                light.position = glm::vec3(pos[0], pos[1], pos[2]);
                            }
                        }
                        else if (light.type == LightComponent::DIRECTIONAL) { // Directional Light
                            // Convert direction vector to pitch and yaw for sliders
                            float pitch = glm::degrees(asinf(-light.direction.y));
                            float yaw = glm::degrees(atan2f(light.direction.x, light.direction.z));

                            bool changed = false;
                            changed |= ImGui::SliderFloat("Pitch", &pitch, -89.9f, 89.9f, "%.1f deg");
                            changed |= ImGui::SliderFloat("Yaw", &yaw, -180.0f, 180.0f, "%.1f deg");

                            if (changed) {
                                // Convert degrees back to radians
                                float pitchRad = glm::radians(pitch);
                                float yawRad = glm::radians(yaw);

                                // Recalculate direction vector
                                glm::vec3 newDirection;
                                newDirection.x = cos(pitchRad) * sin(yawRad);
                                newDirection.y = -sin(pitchRad);
                                newDirection.z = cos(pitchRad) * cos(yawRad);

                                light.direction = glm::normalize(newDirection);
                            }
                        }
                    }
                    else {
                        ImGui::TextDisabled("Selected Light entity is invalid or no LightComponent attached.");
                    }
                }

                static std::string button_text = "Adjust Light with Mouse";

                if (is_adjusting_lights) {
                    button_text = "Stop Adjusting Lights";
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.3f, 0.3f, 1.0f)); // red    
                }else {
                    button_text = "Adjust Light with Mouse";
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.7f, 0.3f, 1.0f)); // green
                }

                if (ImGui::Button(button_text.c_str())) {
                    is_adjusting_lights = !is_adjusting_lights;
                }

                ImGui::PopStyleColor();
                ImGui::EndTabItem();
                // If you want to add new lights
    //            if(ImGui::Button("Add New Light")) {
                //	// Create a new entity with LightComponent
    //                entt::entity new_light_entity = registry.create();
    //                LightComponent lightComp{};

                //	// Set default values
    //                // Position
    //                lightComp.position = glm::vec3(0.0f, 0.0f, 0.0f);

    //                // Ambient Intensity
    //                lightComp.intensity = 1.0f; 

                //	//// Diffuse Intensity
    // //               lightComp.L.push_back(glm::vec3(1.0f));

                //	//// Specular Intensity
    // //               lightComp.L.push_back(glm::vec3(1.0f));
    //                registry.emplace<LightComponent>(new_light_entity, lightComp);
                //}
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

        if (is_simulator_running) return;
        if (is_adjusting_lights) return; //when adjusting lights, gizmos should be disabled
        if (is_adjusting_triggerbox) return;


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

            /*TransformComponent transform{};
            transform.position = object.position;
            transform.rotation = object.rotation;
            transform.scale = object.scale;
            glm::mat4 model{};

            GraphicsSystem::GetInstance().CalculateModelMatrix(transform, model);*/

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

                // Snapping translation
                bool ctrlDown = ImGui::GetIO().KeyCtrl; // While CTRL is held down

                if ((m_snapEnabled || ctrlDown) && operation == ImGuizmo::TRANSLATE)
                {
                    float step = (m_snapStep > 0.0f) ? m_snapStep : 1.0f;

                    auto snap = [step](float v)
                        {
                            return std::round(v / step) * step;
                        };

                    translation[0] = snap(translation[0]);
                    translation[1] = snap(translation[1]);
                    translation[2] = snap(translation[2]);
                }

                // Update the TransformComponent values
                object.position = glm::vec3(translation[0], translation[1], translation[2]);
                object.rotation = glm::vec3(rotation[0], rotation[1], rotation[2]);
                object.scale = glm::vec3(scale[0], scale[1], scale[2]);

                // No need to manually update the modelMatrix as it will be recalculated next frame
                // based on the updated position, rotation, and scale values
            }

            /*
            * History Tracking
            */

            if (!gizmos_started_drag && gizmos_IS_dragged) {

                if (operation == ImGuizmo::TRANSLATE) {
                    current_record = history->start_track(id, "TransformComponent", "position");
                }
                else if (operation == ImGuizmo::SCALE) {
                    current_record = history->start_track(id, "TransformComponent", "scale");
                }
                else if (operation == ImGuizmo::ROTATE) {
                    current_record = history->start_track(id, "TransformComponent", "rotation");
                }

                //std::cout << "Begin track\n";
            }

            if (gizmos_started_drag && !gizmos_IS_dragged) {

                if (operation == ImGuizmo::TRANSLATE) {
                    history->end_track(current_record, object.position);
                }
                else if (operation == ImGuizmo::SCALE) {
                    history->end_track(current_record, object.scale);
                }
                else if (operation == ImGuizmo::ROTATE) {
                    history->end_track(current_record, object.rotation);
                }

                current_record = nullptr;
                //std::cout << "End track\n";
            }
            gizmos_started_drag = gizmos_IS_dragged;
        }


    }

    bool Screen::delete_selected_entity() {

        if (is_simulator_running || is_adjusting_lights) {
            return false;
        }

        if (!editor_ptr) {
            return false;
        }

        entt::entity selected = static_cast<entt::entity>(editor_ptr->selected_objectID);
        if (selected == entt::null || !registry.valid(selected)) {
            return false;
        }

        std::unique_ptr<Record> entity_history{};
        HistorySystem* history_system = history ? history.get() : nullptr;
        if (history_system) {
            entity_history = history_system->start_track(selected, Record_Type::ENTITY_REMOVED);
        }

        registry.destroy(selected);

        if (history_system && entity_history) {
            history_system->end_track(entity_history);
        }

        editor_ptr->selected_objectID = entt::null;
        return true;
    }

    void Screen::entity_shortcut_menu() {

        if (is_simulator_running) return;
        if (is_adjusting_lights) return;

        if (ImGui::BeginPopup("Entity_Shortcut_Menu"))
        {
            entt::entity id = entt::null;
            if (editor_ptr)
            {
                id = static_cast<entt::entity>(editor_ptr->selected_objectID);
            }
            bool has_selection = (id != entt::null) && registry.valid(id);

            if (ImGui::MenuItem("Delete", "Ctrl+Del", false, has_selection)) {
                delete_selected_entity();
            }
            if (ImGui::MenuItem("Duplicate", nullptr, false, has_selection)) {
                entt::entity duplicated = GameObjectSystem::GetInstance().DuplicateInstance(id);
                if (duplicated != entt::null)
                {
                    if (editor_ptr)
                    {
                        editor_ptr->selected_objectID = duplicated;
                        editor_ptr->latest_selected = iEditorView::Latest_Selected::INSTANCE_SELECTED;
                    }
                    if (history)
                    {
                        auto record = history->start_track(duplicated, Record_Type::ENTITY_ADDED);
                        history->end_track(record);
                    }
                }
            }

            /*
            * Adjusting trigger box
            */
            LOGI("[Menu] Selected entity = %u", (unsigned)id);
            //TriggerBoxComponent
            if (has_selection && registry.all_of<TransformComponent, TriggerBoxComponent>(id))
            {
                std::string caption = "Adjust Trigger Box";
                if (is_adjusting_triggerbox) caption = "Stop Adjusting";
                else caption = "Adjust Trigger Box";

                if (ImGui::MenuItem(caption.c_str())) {
                    is_adjusting_triggerbox = !is_adjusting_triggerbox;

                    if (is_adjusting_triggerbox) {
                        LOGI(">> User clicked Adjust Trigger Box for entity %u", (unsigned)id);

                        FitTriggerToMesh(registry, id);
                    }
                }
            }


            ImGui::EndPopup();
        }

    }

    void Screen::adjust_triggerbox() {

        /*
            adjusting trigger box using gizmos
        */
        if (is_simulator_running) return;
        if (is_adjusting_lights) return; //when adjusting lights, gizmos should be disabled
        if (!is_adjusting_triggerbox) return;

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

        //TriggerBoxComponent

        if (registry.all_of<TransformComponent, TriggerBoxComponent>(id))
        {
            auto& transform = registry.get<TransformComponent>(id);
            auto& box = registry.get<TriggerBoxComponent>(id);

            glm::mat4 transform_matrix = transform.modelMatrix;

            glm::quat qLocal = EulerDegToQuat(box.localEulerDeg);
            glm::mat4 localTRS = TRS(box.localPosition, qLocal, box.halfExtents * 2.0f);
            glm::mat4 triggerMatrix = transform_matrix * localTRS;

            glm::mat4 triggerBefore = triggerMatrix; 

            ImGuizmo::BeginFrame();
            ImGuizmo::SetDrawlist(ImGui::GetWindowDrawList());

            
            ImGuizmo::Manipulate(
                glm::value_ptr(view),
                glm::value_ptr(proj),
                operation,
                mode,
                glm::value_ptr(triggerMatrix)
            );
          
            static bool scalingStarted = false;
            static bool shift_start = false;
            static int Axis = -1;
            

            //when dragging for scaling stops
            if (scalingStarted && (!ImGuizmo::IsUsing() || !shift_start))
            {
                scalingStarted = false;
                Axis = -1;
                //std::cout << "dragging stopped\n";
                glm::quat qLocalFinal = EulerDegToQuat(box.localEulerDeg);
                glm::mat4 localTRSFinal = TRS(box.localPosition, qLocalFinal, box.halfExtents * 2.0f);

                triggerMatrix = transform_matrix * localTRSFinal;
            }

            //--------------------------------------------------------------------
            // If gizmo changed the matrix this frame
            //--------------------------------------------------------------------
            if (Check_Mat4_Diff(triggerMatrix, triggerBefore))
            {
                ImGuiIO& io = ImGui::GetIO();
                static glm::vec3 old_Half;  
                static glm::vec3 old_Full;  


                glm::mat4 newLocal = glm::inverse(transform_matrix) * triggerMatrix;

                float t[3], r[3], s[3];
                ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(newLocal), t, r, s);

                glm::vec3 new_Full(s[0], s[1], s[2]);
                glm::vec3 new_Half = new_Full * 0.5f;

                
                if (!scalingStarted && ImGuizmo::IsUsing() && operation == ImGuizmo::SCALE)
                {
                    scalingStarted = true;

                    old_Half = box.halfExtents;
                    old_Full = glm::vec3(old_Half.x * 2.f, old_Half.y * 2.f, old_Half.z * 2.f);

                    Axis = GetDominantScaleAxis((float*)&old_Full, (float*)&new_Full);

                }

                if (scalingStarted && io.KeyShift && ImGuizmo::IsUsing() && operation == ImGuizmo::SCALE)
                {
                    shift_start = true;
                    
                    float ratio = new_Half[Axis] / old_Half[Axis];
                    glm::vec3 scaledHalf = old_Half * ratio;

                    box.halfExtents = scaledHalf;
                }
                else
                {
                    shift_start = false;                 
                    box.halfExtents = new_Half;
                }

                
                box.localPosition = glm::vec3(t[0], t[1], t[2]);
                box.localEulerDeg = glm::vec3(r[0], r[1], r[2]);

                qLocal = EulerDegToQuat(box.localEulerDeg);
                localTRS = TRS(box.localPosition, qLocal, box.halfExtents * 2.0f);
                triggerMatrix = transform_matrix * localTRS;
            }

            DrawWireOBB(triggerMatrix, view, proj, screen_pos, available_size,
                IM_COL32(0, 255, 0, 255), 1.6f);
        }


    }

    void EngineFramework::Screen::triggerbox()
    {
        if (!s_DrawColliders) return;

        if (is_adjusting_lights) return;
        if (is_simulator_running) return;
        

        // Use same camera math as gizmos()
        auto camera_view = registry.view<CameraComponent>();
        if (camera_view.empty()) return;
        auto& camera = camera_view.get<CameraComponent>(camera_view.front());

        float aspect = (GraphicsSystem::GetInstance().GetWindowSize().second > 0)
            ? (float)GraphicsSystem::GetInstance().GetWindowSize().first /
            (float)GraphicsSystem::GetInstance().GetWindowSize().second
            : 1.0f;

        glm::mat4 view = Camera::getLookAt(camera);
        glm::mat4 proj = Camera::getPerspective(camera, aspect);

        // Same ImGuizmo setup rectangle that gizmos() uses
        ImGuizmo::BeginFrame();
        ImGuizmo::SetDrawlist(ImGui::GetWindowDrawList());
        ImGuizmo::SetOrthographic(false);
        ImGuizmo::SetRect(screen_pos.x, screen_pos.y, available_size.x, available_size.y);

        // Draw a wire box for every entity that has a TriggerBoxComponent
        auto v = registry.view<TransformComponent, TriggerBoxComponent>();
        v.each([&](const TransformComponent& tf,
            const TriggerBoxComponent& trg)   // removed entt::entity e (no warning)
            {
                if (!trg.enabled) return;

                const glm::mat4 model = tf.modelMatrix;

                // Local center & size come from the trigger itself
                glm::vec3 centerLocal = trg.localPosition;
                glm::vec3 sizeLocal(1.0f);

                switch (trg.type)
                {
                case TriggerShapeType::Box:
                    sizeLocal = 2.0f * trg.halfExtents;
                    break;
                case TriggerShapeType::Sphere:
                    sizeLocal = glm::vec3(2.0f * trg.radius);
                    break;
                case TriggerShapeType::Capsule:
                    sizeLocal = glm::vec3(2.0f * trg.capRadius,
                        2.0f * (trg.capHalfHeight + trg.capRadius),
                        2.0f * trg.capRadius);
                    break;
                }

                const glm::quat qLocal = EulerDegToQuat(trg.localEulerDeg);

                const glm::mat4 triggerM = model * TRS(centerLocal, qLocal, sizeLocal);

                ImU32 color = trg.highlightedEntities.empty()
                    ? IM_COL32(0, 255, 0, 255)      // green (normal)
                    : IM_COL32(255, 255, 0, 255);   // yellow (highlight)

                DrawWireOBB(triggerM, view, proj, screen_pos, available_size,
                    color, 2.5f);

            });
    }

 
    void EngineFramework::Screen::colliderbox()
    {
        if (!s_DrawColliders) return;

        if (is_adjusting_lights) return;
        if (is_simulator_running) return;

        auto camera_view = registry.view<CameraComponent>();
        if (camera_view.empty()) return;
        auto& camera = camera_view.get<CameraComponent>(camera_view.front());

        float aspect = GraphicsSystem::GetInstance().GetWindowSize().second > 0
            ? (float)GraphicsSystem::GetInstance().GetWindowSize().first /
            (float)GraphicsSystem::GetInstance().GetWindowSize().second
            : 1.0f;

        glm::mat4 view = Camera::getLookAt(camera);
        glm::mat4 proj = Camera::getPerspective(camera, aspect);

        ImGuizmo::BeginFrame();
        ImGuizmo::SetDrawlist(ImGui::GetWindowDrawList());
        ImGuizmo::SetOrthographic(false);
        ImGuizmo::SetRect(screen_pos.x, screen_pos.y, available_size.x, available_size.y);

        entt::entity selected = LevelEditor::GetInstance().variables.selected_objectID;

        auto v = registry.view<TransformComponent, ColliderComponent>();
        v.each([&](entt::entity e, const TransformComponent& tf,
            const ColliderComponent& col)
            {
                if (!s_DrawAllColliders && e != selected)
                {
                    return;
                }

                // If the selected entity lacks collider shapes, render a yellow wireframe using the helper function before skipping the collider drawing block
                if (col.shapes.empty())
                {
                    if (e == selected)
                    {
                        glm::mat4 highlightM;
                        if (BuildSelectionOBB(e, highlightM))
                        {
                            DrawWireOBB(highlightM, view, proj, screen_pos, available_size,
                                IM_COL32(255, 255, 0, 255), 2.0f);
                        }
                    }
                    return;
                }

                const glm::mat4 model = tf.modelMatrix;

                glm::vec3 aabbCenter = (col.minBounds + col.maxBounds) * 0.5f;
                glm::vec3 aabbSize = col.maxBounds - col.minBounds;
                glm::mat4 aabbLocalM = glm::translate(glm::mat4(1.0f), aabbCenter) * glm::scale(glm::mat4(1.0f), aabbSize);
                glm::mat4 aabbWorldM = model * aabbLocalM;
                DrawWireOBB(aabbWorldM, view, proj, screen_pos, available_size, IM_COL32(0, 255, 255, 255), 1.0f);

                glm::vec3 entPos, entScale, entSkew;
                glm::vec4 entPersp;
                glm::quat entRot;
                glm::decompose(model, entScale, entRot, entPos, entSkew, entPersp);

                for (const auto& shape : col.shapes)
                {
                    glm::vec3 sizeLocal(1.0f);

                    switch (static_cast<ColliderShape>(shape.type))
                    {
                    case ColliderShape::Box:
                        sizeLocal = shape.boxHalfExtents * 2.0f;
                        break;
                    case ColliderShape::Sphere:
                        sizeLocal = glm::vec3(shape.sphereRadius * 2.0f);
                        break;
                    case ColliderShape::Capsule:
                        sizeLocal = glm::vec3(
                            shape.capsuleRadius * 2.0f,
                            shape.capsuleHeight,
                            shape.capsuleRadius * 2.0f);
                        break;
                    }
                    glm::vec3 finalSize = sizeLocal * entScale;
                    glm::quat qShape = entRot * EulerDegToQuat(shape.offsetRotation);
                    glm::vec3 finalPos = entPos + (entRot * (shape.offsetPosition * entScale));
                    glm::mat4 shapeTransform = TRS(finalPos, qShape, finalSize);

                    DrawWireOBB(shapeTransform, view, proj, screen_pos, available_size,
                        IM_COL32(255, 0, 0, 255), 1.6f); // red box for collider
                }
                //const ColliderData& shape = col.shapes[0];

                //// only draw boxes for now
                //if (shape.type != (int)ColliderShape::Box)
                //    return;

                //glm::vec3 centerLocal = shape.offsetPosition;
                //glm::vec3 sizeLocal = 2.0f * shape.boxHalfExtents;

                //glm::quat qLocal = EulerDegToQuat(glm::vec3(0.0f)); // if you later use offsetRotation, convert it here
                //glm::mat4 colliderM = model * TRS(centerLocal, qLocal, sizeLocal);

                //DrawWireOBB(colliderM, view, proj, screen_pos, available_size,
                //    IM_COL32(255, 0, 0, 255), 1.6f); // red box for collider
            });

        // If the selected entity doesnt have a collider component, shows a wireframe that highlights regardless of collider data
        if (selected != entt::null &&
            registry.valid(selected) &&
            !registry.all_of<ColliderComponent>(selected))
        {
            glm::mat4 highlightM;
            if (BuildSelectionOBB(selected, highlightM))
            {
                DrawWireOBB(highlightM, view, proj, screen_pos, available_size,
                    IM_COL32(255, 255, 0, 255), 2.0f);
            }
        }
    }

    void Screen::drop_target() {

        if (is_simulator_running) return;
        if (is_adjusting_lights) return;

        if (ImGui::BeginDragDropTarget())
        {
            CALL_AT_END(imgui_enddragdrop, ImGui::EndDragDropTarget(););

            // Create instance with default transform at camera forward position. 
            // Accept path payload (std::string*)
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SELECTED_MODEL_NAME"))
            {
                if (payload->DataSize == 0) return;
                std::string archtype_name = static_cast<const char*>(payload->Data);
                if (archtype_name.empty())
                {
                    LOGD("%s", "Empty Archtype Name provided by Drag-Drop Payload");
                    return;
                }
                TransformComponent instance_transform{};

                //Find the position in front of where the camera is looking.
                entt::entity camera = GraphicsSystem::GetInstance().GetFirstActiveCamera();
                if (camera != entt::null)
                {
                    CameraComponent& cam_comp = registry.get<CameraComponent>(camera);
                    instance_transform.position = Camera::GetFowardLookPosition(cam_comp, 0.5f);

                }//Otherwise if no active camera found then use default position at 0,0,0.
                else
                {
                    LOGD("%s", "No Active Camera Found, creating entity at(0, 0, 0)");
                }

                // Store the entity returned by createinstance so as to assign the selection to the new entity and that the property manager updates right after an archtype is dropped in to viewport
                entt::entity created_entity = GameObjectSystem::GetInstance().CreateInstance(archtype_name, instance_transform);
                if (created_entity != entt::null)
                {
                    LevelEditor::GetInstance().variables.selected_objectID = created_entity;
                    LevelEditor::GetInstance().variables.latest_selected = iEditorView::Latest_Selected::INSTANCE_SELECTED;
                }
            }

        }
    }

    entt::entity Screen::FindPlayerEntity()
    {
        auto view = registry.view<Engine::LuaScriptComponent>();

        for (auto [e, lsc] : view.each())
        {
            for (auto& inst : lsc.Scripts)
            {
                if (inst.ScriptGuid.m_Value == 0)
                    continue;

                // Convert instance GUID to full GUID
                auto full_guid = xresource::full_guid(
                    inst.ScriptGuid,
                    EngineFramework::AssetManagerSystem::SCRIPT_GUID
                );

                // Get the script name from Asset Manager
                std::string scriptName = EngineFramework::AssetManagerSystem::GetName(full_guid);

                // Match with your player script name
                if (scriptName == "player_pov_camera")
                {
                    // Found the player entity!
                    return e;
                }
            }
        }

        return entt::null;
    }

    void Screen::play_simulation() {

        if (is_simulator_running) return;
        //std::cout << "play_simulation\n";
        is_simulator_running = true;

        // ============================
        // SAVE CURRENT CAMERA POSITION
        // ============================
        auto camera_view = registry.view<CameraComponent>();
        if (!camera_view.empty())
        {
            entt::entity camEnt = camera_view.front();
            auto& cam = registry.get<CameraComponent>(camEnt);

            // Save original position
            saved_cam_pos_before_sim = cam.position;

            // Save original target (look-at point)
            saved_cam_target_before_sim = cam.target;

            has_saved_cam_before_sim = true;
        }

        // ===============================================================
        // Teleport Editor Camera to Player Position
        // ===============================================================
        entt::entity player = FindPlayerEntity();

        if (player != entt::null && registry.all_of<TransformComponent>(player))
        {
            auto& playerTF = registry.get<TransformComponent>(player);

            //auto camera_view = registry.view<CameraComponent>();
            if (!camera_view.empty())
            {
                entt::entity camEnt = camera_view.front();
                auto& cam = registry.get<CameraComponent>(camEnt);

                // Set camera position = player position
                Camera::set_Cam_Pos(cam, playerTF.position);

                // Point camera forward (optional)
                glm::vec3 target = playerTF.position + glm::vec3(0, 0, 1);
                Camera::set_Target_Pos(cam, target);

                LOGI("[Simulation] Camera teleported to Player entity %u",
                    static_cast<unsigned>(player));
            }
        }
        else
        {
            LOGE("[Simulation] Player entity not found � ensure player_pov_camera.lua is attached!");
        }

        ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoKeyboard;
        Event_Level_Editor_Simulation editor_toggled{};
        editor_toggled.is_enabled = true;
        event_dispatcher.trigger<Event_Level_Editor_Simulation>(std::move(editor_toggled));


        json_backup = EngineFramework::GameObjectSystem::GetInstance().SerializeAllEntityInstances();

        //get all entity
        //std::vector<entt::entity> entities;

        //// Iterate through the sparse set (all alive entities)
        //for (auto entity : registry.view<entt::entity>()) {
        //    entities.push_back(entity);
        //}

        //for (auto e : entities) {

        //    Entity_Info e_info{};


        //    for (auto&& storage : GameObjectSystem::GetInstance().GetRegistry().storage())
        //    {
        //        if (!storage.second.contains(e))
        //            continue;

        //        entt::meta_type meta = entt::resolve(storage.second.info());
        //        if (!meta)
        //            continue;

        //        std::string compName = meta.name();
        //        if (compName.empty())
        //            continue;

        //        std::cout << "compName " << compName << std::endl;
        //        
        //        // Save normal component with fields
        //        entt::meta_any comp_ref =
        //            meta.func("get"_hs).invoke({}, e);

        //        bool hasData = false;

        //        for (auto&& [id, member] : meta.data())
        //        {
        //            hasData = true;
        //            std::string varName = member.name();
        //            entt::meta_any value = member.get(comp_ref);
        //            e_info.components_variables[compName][varName] = value;
        //        }

        //        if (!hasData) {
        //            e_info.components_variables[compName] ={};
        //        }
        //    }

        //    backup_before_simulation[e] = e_info;
        //}


    }
    void Screen::stop_simulation() {

        if (!is_simulator_running) return;
        //std::cout << "stop_simulation\n";

        ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NoKeyboard;
        GameObjectSystem::GetInstance().ClearAllInstances();

        GameObjectSystem::GetInstance().LoadScene(json_backup);

        json_backup = {};

        Event_Level_Editor_Simulation editor_toggled{};
        editor_toggled.is_enabled = false;
        event_dispatcher.trigger<Event_Level_Editor_Simulation>(std::move(editor_toggled));

        //for (std::map<entt::entity, Entity_Info>::iterator it = backup_before_simulation.begin(); it != backup_before_simulation.end(); it++) {
        //    entt::entity e = it->first;
        //    Entity_Info comp = it->second;

        //    if (!GameObjectSystem::GetRegistry().valid(e)) {
        //        entt::entity en = GameObjectSystem::GetRegistry().create(e);
        //        UNREFERENCED_PARAMETER(en);

        //       
        //    }
        //    else {
        //        continue;
        //    }

        //    for (std::map<std::string, std::map<std::string, entt::meta_any>>::iterator comp_it = comp.components_variables.begin();comp_it != comp.components_variables.end(); comp_it++){
        //        const std::string& component_name = comp_it->first;
        //        std::map<std::string, entt::meta_any>& variables = comp_it->second;

        //        if (component_name == "archtype") continue;

        //        if (component_name == "SceneRenderPass") {
        //            std::cout << "component_name: " << component_name << std::endl;
        //        }
        //        
        //       
        //        //if (component_name == "archtype") continue;
        //        auto meta = entt::resolve(entt::hashed_string{ component_name.c_str() }.value());

        //        if (!is_meta_component(meta))
        //            continue;

        //        if (variables.empty())
        //        {
        //            // Just emplace it into the entity.
        //            auto emplace = meta.func("emplace"_hs);
        //            if (emplace) {
        //                emplace.invoke({}, e, nullptr);
        //            }

        //            continue;
        //        }

        //        entt::meta_any instance = meta.construct();

        //        for (std::map<std::string, entt::meta_any>::iterator var_it = variables.begin(); var_it != variables.end(); var_it++)
        //        {
        //            const std::string& var_name = var_it->first;
        //            entt::meta_any& saved_value = var_it->second;

        //            auto data = meta.data(entt::hashed_string{ var_name.c_str() }.value());
        //            if (!data)continue;
        //            data.set(instance, saved_value);
        //        }

        //        auto emplace = meta.func("emplace"_hs);
        //        if (emplace) {
        //            entt::meta_any result = emplace.invoke({}, e, static_cast<const void*>(&instance));
        //        }
        //        
        //    }
        //}

        // ============================
        // RESTORE PREVIOUS CAMERA POSE
        // ============================
        if (has_saved_cam_before_sim)
        {
            auto camera_view = registry.view<CameraComponent>();
            if (!camera_view.empty())
            {
                entt::entity camEnt = camera_view.front();
                auto& cam = registry.get<CameraComponent>(camEnt);

                Camera::set_Cam_Pos(cam, saved_cam_pos_before_sim);
                Camera::set_Target_Pos(cam, saved_cam_target_before_sim);

                LOGI("[Simulation] Camera restored to original editor position.");
            }

            has_saved_cam_before_sim = false;
        }

        is_simulator_running = false;

    }

    void Screen::clear_backup() {

        /*for (auto& [e, info] : backup_before_simulation) {

            for (auto& [comp, vars] : info.components_variables) {
                for (auto& [var, val] : vars) {
                    val.reset();
                }
            }

            info.components_variables.clear();
        }

        backup_before_simulation.clear();*/

    }

}
#endif
