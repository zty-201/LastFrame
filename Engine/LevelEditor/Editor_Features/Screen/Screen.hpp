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

namespace EngineFramework
{
    struct iEditorView; //forward declaration

    class Screen : public LFeature
    {
    public:

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
        

    private:

        std::unique_ptr<HistorySystem> history;
        void display_screen();
        void gizmos();
        void camera_controls_outside_screen();
        void camera_controls_in_screen_only();
        void control_window();
        void drop_target();

        void play_simulation();
        void stop_simulation();
        std::string simulation_buffer;
        static inline Screen* this_instance{};
    };

}




#endif
#endif
