#ifndef ARCHETYPE_HPP_A
#define ARCHETYPE_HPP_A
#include "entt/meta/meta.hpp"
#include "entt/entity/entity.hpp"
namespace EngineFramework
{
	/*
		Archtype class used to contain component data. Not stored in the entity system.

		Note:
		1. Ensure the component has a valid copy operator/ctor as it'll be copied from archtype into entity.
		2. The component destructor will be called when this archtype is freed
		- Only data types (not pointers) will be set by the archtype.
	*/
	class Archtype
	{
	public:
		Archtype(const std::string& json_data);
		//Returns a json string representing components and their data.
		std::string Serialize() const;

		//Component data
		std::vector<entt::meta_any> component_arr;
		//Label, used by model_manager in level editor to indicate what type of archtype it is.
		//e.g. "Characters" "Background Objects"
		//Use a string to be easily customisable. 
		std::string model_type{};
	};
}



#endif