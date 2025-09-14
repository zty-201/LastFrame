
/*

		Uses Entt as the backbone.
*/
#ifndef SYSTEM_LEVEL_EDITOR_HPP
#define SYSTEM_LEVEL_EDITOR_HPP
#include <unordered_map>
#include <string>
#include <sstream>
#include "ISystem.hpp"
#include "entt/entity/registry.hpp"
#include "Level_Editor.hpp"

namespace EngineFramework
{

	/*

	*/
	class LevelEditorSystem : public ISystem
	{
	public:
		LevelEditorSystem();
	private:
		void Initialize() override;
		void Update(float delta_time) override;

		//Keeps track of the one and only instance of this system, created by core engine.
		static inline LevelEditorSystem* this_system{};

		std::unique_ptr<LevelEditor> application;

	};


}




#endif