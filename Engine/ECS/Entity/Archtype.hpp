#ifndef ARCHETYPE_HPP_A
#define ARCHETYPE_HPP_A
#include "entt/meta/meta.hpp"
#include "entt/entity/entity.hpp"
namespace EngineFramework
{
	class Archtype
	{
	public:
		Archtype(const std::string& json_data);
		entt::entity Instantiate();
	private:
		std::vector<entt::meta_any> component_arr;
	};
}



#endif