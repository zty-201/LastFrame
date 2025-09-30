#include "Archtype.hpp"
#include "entt/meta/resolve.hpp"
#include "entt/core/hashed_string.hpp"
#include "nlohmann/json.hpp"
#include "ReflectionRegister.hpp"

namespace EngineFramework
{
	/*
		Creating Archtype
		1. Create meta_any for each json component type string.
		2. Call entt::meta_any::set for each json data type string, with the associated value.
		3. Store meta_any

		Using Archtype
		1. For each meta_any in the container, invoke the emplace function (created during registration) to cast it to T and emplace into entity.

		Reading Archtype
		1. iterate through each meta_any
		2. iterate through each data member, and use entt::meta_type.isX() to check what type it is.
		3. using that, get a ptr to the data member and use that.
	*/
	using namespace entt::literals;
	Archtype::Archtype(const std::string& json_data)
	{
		/*
		* 	Creating Archtype
			1. Create meta_any for each json component type string.
			2. Call entt::meta_any::set for each json data type string, with the associated value.
			3. Store meta_any
		*/

		//Iterate over components
		nlohmann::json data = nlohmann::json::parse(json_data);
		for (auto& [comp_name, comp_data_json] : data.items())
		{
			//Creation of component.
			entt::meta_type comp_type = entt::resolve(GetID(comp_name));
			entt::meta_any comp_data = comp_type.construct();
			//No data members, leave as empty and continue;
			if (comp_data_json.empty())
			{
				component_arr.emplace_back(comp_data);
				continue;
			}

			//Iterate over component data members and set values.
			for (auto& [data_name, data_val_json] : comp_data_json.items())
			{
				entt::id_type data_type_id = GetID(data_name);
				entt::meta_data data_type = comp_type.data(data_type_id);
				//Ensure it is a data member. It could be a function for example.
				if (!data_type) continue;
				//Set the data member.
				SetMetaAnyWithJson(comp_data, data_type, data_val_json);
			}
			//Store component
			component_arr.emplace_back(comp_data);
		}
	}
	entt::entity Archtype::Instantiate()
	{
		entt::entity entity = registry.create();
		//Emplace all components inside the entity.
		//Each component must have an "emplace" lambda/function registered.
		for (entt::meta_any& comp : component_arr)
		{
			auto func = comp.type().func("emplace"_hs);
			if (!func)
			{
				throw std::exception("Emplace function not registered for this component");
			}
			func.invoke({}, entity, &comp);
		}


		return entity;
	}

}