/*
		Controls serialization, deserialization, safe deletion of instances.
		Also controls serialization and deserialization for archtypes.

		Uses Entt as the backbone library.
*/
#include "pch.hpp"

//File management, JSON file parser/writer
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

//Entt reflection system
#include <entt/meta/meta.hpp>
#include <entt/meta/resolve.hpp>
//Entt ECS system
#include <entt/entity/registry.hpp>
#include "System_GameObjectSystem.hpp"

//Casting meta data to type.
#include "ReflectionRegister.hpp"
#include "Archtype.hpp"

namespace EngineFramework
{

	GameObjectSystem::GameObjectSystem()
	{
		this_system = this;

	}
	GameObjectSystem& GameObjectSystem::GetInstance()
	{
		return *this_system;
	}

	void GameObjectSystem::LoadScene(const std::string& json_input)
	{
		ClearAllInstances();
		DeSerializeAllEntityInstances(json_input);
	}

	void GameObjectSystem::ClearAllInstances()
	{
		registry.clear();
	}
	void GameObjectSystem::Initialize()
	{
		////TODO: remove
		//LOGD("%s", "Testing deserialization of archetype");
		//std::vector<unsigned char> data = EngineFramework::AssetManagerSystem::GetDataFromFile(std::filesystem::path("Extern/Archtypes/testclass.json"));
		//std::string test_string{ data.begin(), data.end() };
		//Archtype a{ test_string };
		//a.Instantiate();
		//a.Instantiate();

		//auto view = registry.view<EngineFramework::TODELETE>();
		//for (auto entity : view) {
		//	TODELETE& comp = view.get<EngineFramework::TODELETE>(entity);
		//	LOGI("%s", comp.c.c_str());
		//}

	}
	void GameObjectSystem::Update(float delta_time)
	{
		UNREFERENCED_PARAMETER(delta_time);

	}

	

	/*
		Save gameobject archtypes to files using reflection.
		Each archtype is saved to their own respective files with their own respective names, in the folder.
	*/
	std::vector<std::pair<std::string, std::string>> GameObjectSystem::SerializeAllEntityArchtypes()
	{
		//Each pair represents <archtype_name, archtype_json_data>
		std::vector<std::pair<std::string, std::string>> save_data{};
		/*
			Iterate over all archtypes, save their component data and registered members to json.

			json format (archtype name is the file name).
			{
				"component_name1":
				{
					"data_name1" : "value",
					"data_name2" : value,
					"data_name3" : [
						2, 3, 4
					]
				},
				"component_name2"
				{
					...
				}
			}
		*/
		for (const std::pair<std::string, Archtype>& archtype_pair : archtype_arr)
		{
			std::string archtype_name = archtype_pair.first;
			const Archtype& archtype_data = archtype_pair.second;
			nlohmann::json archtype_json_data{};
			/*
				Iterate over all component types and add each member
			*/
			for (const entt::meta_any& comp : archtype_data.component_arr)
			{
				//Store data members of the component, then add the component json under archtype_data.
				nlohmann::json component_json_data{};
				//Iterate over all data members.
				for
			}
		}


		//std::filesystem::path folder_path_string{ folder_path };
		////Not a folder.
		//if (!std::filesystem::is_directory(folder_path_string))
		//{
		//	throw std::invalid_argument(std::string("No such folder path when serializing in GameObjectSystem: ") + folder_path);
		//}
		//
		////Iterate over every archtype and store their component data in their respective folders.
		//for (std::pair<const std::string, entt::entity>& archtype : archtype_entities)
		//{
		//	//Empty json object.
		//	nlohmann::json archtype_data;
		//	/*
		//		Store
		//		- Component name, and their data members. Do for all components
		//		- entt::entity id (may be useful for remapping and maintaining relationships between parent and child gameobjects.
		//		
		//		Do not store
		//		- Archtype name (it'll be store as their file name)
		//	*/

		//	/*
		//		Iterate over each component type.
		//		For each component type, check if the entity has it.
		//		If so, save it.

		//		<type_id, sparse_set> where type_id represents the component type.

		//		Note: Cannot simply iterate over all components for an entity, as components are not 
		//		stored in the entity but rather stored in their own pools based on type.
		//	*/
		//	for (auto&& curr : registry.storage())
		//	{
		//		//If an entity does not contain this specific component type, continue to the next type.
		//		if (!curr.second.contains(archtype.second))
		//		{
		//			continue;
		//		}
		//		//Save the component as a json object.
		//		nlohmann::json component_data;
		//		/*
		//			Component name
		//			->Name of data member, value
		//		*/
		//		entt::meta_type component_type = entt::resolve(curr.first);
		//		std::string component_name = component_type.name();
		//		//==Iterate over each data type in the component.
		//		for (auto&& member_data : component_type.data())
		//		{
		//			//Add the member data (can be most data types) to the component json object, together with the member name. 
		//			entt::meta_data& meta_data = member_data.second;
		//			std::string meta_name = meta_data.name();
		//			CastMetaDataToType(meta_data, [&meta_name, &component_data](auto val) {
		//				component_data[meta_name] = val;
		//				});
		//		}

		//		//Add the component object as part of the archtype object.
		//		archtype_data[component_name] = component_data;
		//	}

		//	//Create a file path to the archtype.
		//	std::filesystem::path file_path = folder_path_string;
		//	file_path /= archtype.first.c_str();


		//	std::ofstream archtype_file(file_path);
		//	//TODO: Write the json archtype data to file.
		//}
	}

	/*
		Read gameobject archtypes from files using reflection.
		Each archtype is read from their own respective files with their own respective names, in the folder.
		After construction, each entity id is stored together with its name inside the archtype entities map.

		Do once at the start of the program.
	*/
	void GameObjectSystem::DeserializeAllEntityArchtypes(const std::vector<std::pair<std::string, std::string>>& json_input)
	{
		for (const std::pair<std::string, std::string>& name_json_pair : json_input)
		{
			//There should not already be an archtype of the same name inside.
			if (archtype_arr.find(name_json_pair.first) != archtype_arr.end())
			{
				LOGE("%s %s", "Archtype already present in array, duplicate deserialize detected: ", name_json_pair.first.c_str());
				continue;
			}
			archtype_arr.emplace(name_json_pair.first, name_json_pair.second);
		}
	}

	/*
		Save gameobject instances to a specified level file using reflection.
	*/
	std::string GameObjectSystem::SerializeAllEntityInstances()
	{
		//TODO:
		return {};
	}

	/*
		Read gameobject instances from a specified level file using reflection.
		Before this, all archtypes need to be read.
	*/
	void GameObjectSystem::DeSerializeAllEntityInstances(const std::string& json_input)
	{
		UNREFERENCED_PARAMETER(json_input);
	}

	/*
			Creates a single entity with the same component data (copied) from archtype
		*/
	entt::entity GameObjectSystem::CreateInstance(std::string archtype_str)
	{
		//Get archtype
		auto archtype_it = archtype_arr.find(archtype_str);
		if (archtype_it == archtype_arr.end())
		{
			throw std::runtime_error((std::string("Archtype used for instantiating instance is not valid: ") + archtype_str).c_str());
		}
		//Create instance from archtype component data.
		Archtype& archtype = archtype_it->second;
		return archtype.Instantiate(); //Modifies global registry
	}
}