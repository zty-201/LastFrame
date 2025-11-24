/*
*****************************************************************/
/*!
\file   Screen.hpp
\author Tan Yee Tong
\par    yeetong.t\@digipen.edu
\date   September 2025
\brief
This file enables the screen buffer of the level editor to run
Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* *******************************************************************/

#if defined(PLATFORM_WINDOWS) && defined(_DEBUG)
#ifndef SCREEN_LFEATURE_HPP
#define SCREEN_LFEATURE_HPP
#include <unordered_map>
#include <string>
#include <sstream>
#include "ISystem.hpp"
#include "entt/entity/registry.hpp"

#include "System_LevelEditor.hpp"
#include "LFeature.hpp"
#include "HistorySystem.hpp"
#include "Editor_Features/CameraController/EditorCameraController.hpp"

namespace EngineFramework
{
    struct iEditorView; //forward declaration

    /*struct Entity_Info {
        xresource::instance_guid archtype_guid{};
        std::map <std::string, std::map<std::string, entt::meta_any>> components_variables;
    };*/

    class Screen : public LFeature
    {
    public:

        //bool is_meta_component(const entt::meta_type& meta) {
        //    if (!meta || !meta.is_class()) {
        //        return false;
        //    }
        //    using namespace entt::literals;
        //    return meta.func("emplace"_hs) && (/*meta.func("get_ref"_hs) ||*/ meta.func("get"_hs));
        //}

        Screen();
        ~Screen() override;

        void Initialize() override;
        void Run() override;
        void Close() override;

        static Screen& GetInstance();

        static bool is_control_open;
        static bool is_simulator_running;
        static bool gizmos_IS_hovering;
        static bool gizmos_IS_dragged;
        static bool is_adjusting_lights;
        static bool is_adjusting_triggerbox;

        glm::vec3 saved_cam_pos_before_sim{};
        glm::vec3 saved_cam_target_before_sim{};
        bool has_saved_cam_before_sim = false;

    private:

        std::unique_ptr<HistorySystem> history;
        void display_screen();
        void gizmos();
        void triggerbox();
        void camera_controls_outside_screen();
        void camera_controls_in_screen_only();
        void control_window();
        void drop_target();
        void adjust_triggerbox();
        void colliderbox();
        void entity_shortcut_menu();

        float camera_speed;
        float scrolling_speed;

		entt::entity FindPlayerEntity();
        bool m_snapEnabled = false;
        float m_snapStep = 1.0f;

        void play_simulation();
        void stop_simulation();
        std::string simulation_buffer;
        EditorCameraController m_editorCamera;
        /* std::map<entt::entity, Entity_Info> backup_before_simulation;*/

        void clear_backup();
        static std::string json_backup;
        static inline Screen* this_instance{};
    };

}




#endif
#endif
