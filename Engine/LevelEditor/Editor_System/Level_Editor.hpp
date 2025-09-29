#include "pch.hpp"
#if defined(PLATFORM_WINDOWS) && defined(_DEBUG)
#ifndef LEVEL_EDITOR_HPP
#define LEVEL_EDITOR_HPP

#include <unordered_map>
#include <string>
#include <sstream>
#include <map>
#include <memory>
#include "entt/entity/registry.hpp"
#define IMGUI_IMPL_OPENGL_ES3
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "LFeature.hpp"

namespace EngineFramework
{
	// Forward declaration to be used in unique pointer (Testing for imgui profiler)
	class LFeature;

	//for 1 time init to be used in run()
	struct Imgui_const_Flags {

		ImGuiWindowFlags DockWindowFlags;
	};

	//limiting the accessable variables for Lfeature, only able to change certain variables
	struct iEditorView {

		int selected_objectID;
		int selected_modelID;
	};

	class LevelEditor
	{
	public:

		LevelEditor();
		~LevelEditor();

		static bool Get_Editor_State() {
			return is_enabled;
		}

		bool To_Close() const {
			return to_close;
		}

		iEditorView variables;

		//no point checking state change in every frame, reduce performance, only toggle when needed
		
		void Initialize(); //init imgui/glfw during startup
		void Open(); //open the app
		void Run(); //run the app
		void Close(); //close the app
		void Clean(); //destory any assets when needed, to prevent mem leak (when window is exited)

	private:
		//Keeps track of the one and only instance of this system, created by core engine.
		static inline LevelEditor* this_system{};
		static bool is_enabled;
		bool to_close;
		//bool to_open;
		Imgui_const_Flags imgui_flags;
		std::map<std::string, std::unique_ptr<LFeature>> featureMap;

		void main_window();
		void docking();
		void addFeature(const std::string&, std::unique_ptr<LFeature>, bool enabled = false); //sub_menus/window or functions
		const bool& featureFlag(const std::string& name) const; //flag_state read only
		bool& featureFlag(const std::string& name); //flag_state

	};


}




#endif
#endif

