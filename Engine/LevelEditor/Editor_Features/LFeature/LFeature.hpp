/*
		
		Uses Entt as the backbone.
*/
#ifndef LEVEL_EDITOR_LFEATURE_HPP
#define LEVEL_EDITOR_LFEATURE_HPP
#include <unordered_map>
#include <string>
#include <sstream>
#include "ISystem.hpp"
#include "entt/entity/registry.hpp"

#include "System_LevelEditor.hpp"
#include "Level_Editor.hpp"


namespace EngineFramework
{

	class LFeature
	{
	public:
		LFeature();
		
		void Initialize();
		void Run();
		void Close();
		//void Update(float delta_time);

	};


}




#endif