#include "pch.hpp"
#include "Archtype.hpp"
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
	

	std::string Archtype::Serialize() const
	{
		//	json format (archtype name is the file name).
		//	{
		//		"component_name1":
		//		{
		//			"data_name1" : "value",
		//			"data_name2" : value,
		//			"data_name3" : [
		//				2, 3, 4
		//			]
		//		},
		//		"component_name2"
		//		{
		//			...
		//		}
		//	}
		//*/
		nlohmann::json archtype_json_data{};
		/*
			Iterate over all component types and add each member
		*/
		for (const entt::meta_any& comp : component_arr)
		{
			//Store data members of the component, then add the component json under archtype_data (nested json).
			nlohmann::json component_json_data{};
			entt::meta_type comp_type = comp.type();
			//Iterate over all data members in the component, forming <name, value> pair for each.
			for (const auto& data_member : comp_type.data())
			{
				entt::meta_any member_data = comp.get(data_member.first);
				//Set Json with member_data, by casting it to the type determined by data_member.second
				SetJsonWithMetaAny(component_json_data, data_member.second, member_data);
			}
			//Add component json to archtype json.
			archtype_json_data[comp.type().name()] = component_json_data;
		}
		//Dump in a prettified/readable format (using 1 tab for indentation).
		return archtype_json_data.dump(1, '\t');
	}
}