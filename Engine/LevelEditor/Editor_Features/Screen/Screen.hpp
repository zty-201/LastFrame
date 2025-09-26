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

        static bool control_open;
        static bool simulator_running;
    private:
        void display_screen();
        void camera_controls_outside_screen();
        void camera_controls_in_screen_only();
        void control_window();
         
    };

}




#endif
#endif
