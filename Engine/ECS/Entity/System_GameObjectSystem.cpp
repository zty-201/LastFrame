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
#include "Archtype.hpp"
#include "ReflectionRegister.hpp"

#include "AssetManager.hpp"

//For printing instance/type_guids in error messages
#include <inttypes.h>



namespace EngineFramework
{

	using namespace entt::literals;
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
		
	}
	void GameObjectSystem::Update(float delta_time)
	{
		UNREFERENCED_PARAMETER(delta_time);

		//TODO: Testing, to delete.
		//std::vector<unsigned char> test_dummy_scene = AssetManagerSystem::GetDataFromFile("Extern/Scenes/Dummy1.json");
		//std::string test_scene{ test_dummy_scene.begin(), test_dummy_scene.end() };
		//DeSerializeAllEntityInstances(test_scene);
	}



	/*
		Save gameobject archtypes to files using reflection.
		Each archtype is saved to their own respective files with their own respective names, in the folder.
	*/
	std::vector<std::pair<std::string, std::string>> GameObjectSystem::SerializeAllEntityArchtypes()
	{
		//Get the archtypes stored in asset manager.
		std::vector<std::pair<xresource::instance_guid, std::shared_ptr<Archtype>>> archtype_arr = AssetManagerSystem::GetAllResourceOfType<Archtype>(AssetManagerSystem::ARCHTYPE_GUID);
		//Each pair represents <archtype_name, archtype_json_data>
		std::vector<std::pair<std::string, std::string>> save_data{};
		//	Iterate over all archtypes, save their component data and registered members to json.

		for (const auto& archtype_pair : archtype_arr)
		{
			std::string archtype_name = AssetManagerSystem::GetName(archtype_pair.first);
			if (archtype_name.empty() || !archtype_pair.second) continue; //Cannot serialize an archtype with no name or value.
			//Add this archtype to the array.
			//<archtype name, archtype_json_string> pair.
			save_data.emplace_back(std::make_pair(archtype_name, archtype_pair.second->Serialize()));
		}

		return save_data;
	}

	/*
		Save gameobject instances to a specified level file using reflection.
	*/
	std::string GameObjectSystem::SerializeAllEntityInstances()
	{
		


		//TODO: May want to keep a map<entityid, archtype_name_string> to map instances back to their archtype when serializing. this can be easily changeable. 
		//Each instance has the archtype name (can be left empty if not based on archtype), and other component data.

		//TODO:
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
		return {};
	}

	/*
		Read gameobject instances from a specified level file using reflection.
		Before this, all archtypes need to be read.
	*/
	void GameObjectSystem::DeSerializeAllEntityInstances(const std::string& json_input)
	{
		nlohmann::json json_scene = nlohmann::json::parse(json_input);
		/*
			Deserialization process is similar to archtype.
			- It is instantiated from an archtype (if none, then an entity is simply created)
			- Additional component data can be read, where they are first read into a meta_any object
			before its registered emplace function is called.
		*/
		for (auto& [key, entity_json] : json_scene.items())
		{
			/*
				1. Instantiate from archtype, or from scratch otherwise.
			*/
			bool is_instantiated = false;
			entt::entity entity_id{};
			if (entity_json.contains("archtype"))
			{
				xresource::instance_guid archtype_guid{ entity_json["archtype"].get<uint64_t>() };
				std::shared_ptr<Archtype> archtype = AssetManagerSystem::GetResource<Archtype>({ archtype_guid, AssetManagerSystem::ARCHTYPE_GUID });
				if (archtype)
				{
					is_instantiated = true;
					entity_id = GameObjectSystem::Instantiate(archtype);
				}
				else
				{
					//Fail silently, in the case where archtype is not loaded and/or is deleted. Instance is loaded without the archtype.
					LOGE("[GameObjectSystem] Deserialization: No archtype loaded for %" PRIu64 "guid, instance cannot be instantiated from it", archtype_guid.m_Value);
				}
			}
			//If not created from archtype, then simply create an empty entity.
			if (!is_instantiated)
			{
				entity_id = registry.create();
			}
			//Iterate over additional components for that entity and emplace them into the entity (registry).
			for (auto& [comp_name, comp_data_json] : entity_json.items())
			{
				//Already read the archtype. it's not a component to emplace.
				if (comp_name == "archtype") continue;
				//Creation of component.
				entt::meta_type comp_type = entt::resolve(GetID(comp_name));
				entt::meta_any comp_data = comp_type.construct();

				//Iterate over component data members and set values. If empty it does nothing.
				for (auto& [data_name, data_val_json] : comp_data_json.items())
				{
					entt::id_type data_type_id = GetID(data_name);
					entt::meta_data data_type = comp_type.data(data_type_id);
					//Ensure it is a data member. It could be a function for example.
					if (!data_type) continue;
					//Set the data member.
					SetMetaAnyWithJson(comp_data, data_type, data_val_json);
				}
				
				//==Emplace component data into map, by calling the "emplace" function registered with this type. All components must have emplace function.
				auto func = comp_data.type().func("emplace"_hs);
				if (!func)
				{
					throw std::exception((std::string("[GameObjectSystem] Emplace function not registered for this component: ") + comp_data.type().name()).c_str());
				}
				entt::meta_any success = func.invoke({}, entity_id, &comp_data);
				//Duplicate component already present inside.
				if (!success.cast<bool>())
				{
					//Silently give an error. 
					LOGE("[GameObjectSystem] Duplicate component %s already present in Instance, will be ignored", comp_data.type().name());
				}
			}

		}
	}

	entt::entity GameObjectSystem::Instantiate(std::shared_ptr<Archtype> archtype)
	{
		entt::entity entity = registry.create();
		//Emplace all components inside the entity.
		//Each component must have an "emplace" lambda/function registered.
		for (const entt::meta_any& comp : archtype->component_arr)
		{
			auto func = comp.type().func("emplace"_hs);
			if (!func)
			{
				throw std::exception((std::string("[GameObjectSystem] Emplace function not registered for this component: ") + comp.type().name()).c_str());
			}
			entt::meta_any success = func.invoke({}, entity, &comp);
			//Duplicate component already present inside.
			if (!success.cast<bool>())
			{
				//Silently give an error. 
				LOGE("[GameObjectSystem] Duplicate component %s already present in Archtype, will be ignored", comp.type().name());
			}
		}


		return entity;
	}

	/*
			Creates a single entity with the same component data (copied) from archtype
		*/
	entt::entity GameObjectSystem::CreateInstance(std::string archtype_str)
	{
		//Get archtype
		std::shared_ptr<Archtype> archtype = AssetManagerSystem::GetResource<Archtype>(AssetManagerSystem::GetFullGuid(AssetManagerSystem::ARCHTYPE_GUID, archtype_str));
		if (!archtype)
		{
			//Fail silently, in the case where archtype is not loaded and/or is deleted. Instance is loaded without the archtype.
			LOGE("[GameObjectSystem] Deserialization: No archtype loaded for %s, instance cannot be instantiated from it", archtype_str.c_str());
			return registry.create(); //Create and return empty entity object.
		}
		//Create instance from archtype component data.
		return Instantiate(archtype);
	}
}