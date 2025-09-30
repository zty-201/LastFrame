#ifndef ARCHETYPE_HPP_A
#define ARCHETYPE_HPP_A
#include "entt/meta/meta.hpp"
#include "entt/entity/entity.hpp"
namespace EngineFramework
{
	/*
		Archtype class used to instantiate entity instances in the global entt::registry with copied component data.

		Note:
		1. Ensure the component has a valid copy operator/ctor as it'll be copied from archtype into entity.
		2. The component destructor will be called when this archtype is freed
		- Only data types (not pointers) will be set by the archtype.

		Call Instantiate to automatically emplace entity into registry and set its components.
	*/
	class Archtype
	{
	public:
		Archtype(const std::string& json_data);
		entt::entity Instantiate() const;
		//Returns a json string representing components and their data.
		std::string Deserialize() const;
		std::vector<entt::meta_any> component_arr;
	};
}



#endif