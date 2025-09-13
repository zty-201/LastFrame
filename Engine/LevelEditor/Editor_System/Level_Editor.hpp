
/*

		Uses Entt as the backbone.
*/
#ifndef LEVEL_EDITOR_HPP
#define LEVEL_EDITOR_HPP
#include <unordered_map>
#include <string>
#include <sstream>
#include "entt/entity/registry.hpp"


namespace EngineFramework
{

	class LevelEditor
	{
	public:
		LevelEditor();
		
		bool Get_Editor_State() const {
			return is_enabled;
		}
		bool To_Close() const {
			return to_close;
		}
		bool To_Open() const {
			return to_open;
		}

		//no point checking state change in every frame, reduce performance, only toggle when needed
		void Toggle(); //input manager should call this function
		void Initialize(); //init imgui/glfw during startup
		void Open(); //open the app
		void Run(); //run the app
		void Close(); //close the app
		void Clean(); //destory any assets when needed, to prevent mem leak (when window is exited)

	private:
		//Keeps track of the one and only instance of this system, created by core engine.
		static inline LevelEditor* this_system{};
		bool is_enabled;
		bool to_close;
		bool to_open;

	};


}




#endif