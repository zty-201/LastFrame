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

// Game Object Components
#include "TransformComponent.h"
#include "MeshComponent.h"
#include "MaterialComponent.h"
#include "LightComponent.h"
#include "CameraComponent.h"
#include "TextureComponent.h"
#include "NormalMapComponent.h"
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
		//Clear map as there's no longer any point to saving instance --> archtype mapping if the instances are gone.
		entity_archtype_map.clear();
	}
	void GameObjectSystem::Initialize()
	{
		// Load a Scene
		std::vector<unsigned char> start_scene_data = AssetManagerSystem::GetDataFromFile("Extern/Scenes/Dummy1.json");
		std::string start_scene{ start_scene_data.begin(), start_scene_data.end() };
		LoadScene(start_scene);
		
	}
	void GameObjectSystem::Update(float delta_time)
	{
		UNREFERENCED_PARAMETER(delta_time);

		////TODO: Testing, to delete.
		//std::vector<unsigned char> test_dummy_scene = AssetManagerSystem::GetDataFromFile("Extern/Scenes/Dummy1.json");
		//std::string test_scene{ test_dummy_scene.begin(), test_dummy_scene.end() };
		////DeSerializeAllEntityInstances(test_scene);
		//auto view = registry.view<TODELETE>();
		//auto view2 = registry.view<TODELETE2>();
		//for (entt::entity entity_id : view)
		//{
		//	TODELETE& comp = view.get<TODELETE>(entity_id);
		//	//LOGD("[Gameobjectsystem update] %s", comp.c.c_str());
		//	UNREFERENCED_PARAMETER(comp);
		//}
		//for (entt::entity entity_id : view2)
		//{
		//	TODELETE2& comp = view2.get<TODELETE2>(entity_id);
		//	//LOGD("[Gameobjectsystem update] %s", comp.c.c_str());
		//	UNREFERENCED_PARAMETER(comp);
		//}
		////std::string test_save_data = SerializeAllEntityInstances();
		////LOGD("Testing Serialization: %s", test_save_data.c_str());
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
		/*
		{
			"1": {
				"archtype": 8960202118185140225,
					"TestClass2" : {
					"player_damage": 12,
						"e" : 15.0,
						"c" : "testclass2.1"
				}
			},
				"2": {
				"TestClass": {
					"player_health": 123,
						"b" : 1234.15,
						"c" : "testclass1.2"
				},
					"TestClass2" : {
					"player_damage": 15,
						"e" : 16.0,
						"c" : "testclass2.2"
				}
			}
		}
		*/
		/*
			Entities
			-> Archtype (optional)
			-> Component
			--> Component data
		*/
		nlohmann::json save_data{};
		int id{}; //Basic id to prevent duplicate keys in json. Does not represent entity id.
		// Iterate over all entities
		for (auto entity : registry.view<entt::entity>()) {
			nlohmann::json entity_save_data{};
			static std::vector<entt::meta_type> archtype_components{};
			archtype_components.clear();
			//Store the associated archtype if available.
			if (entity_archtype_map.contains(entity))
			{
				//Find the archtype components.
				xresource::instance_guid archtype_guid = entity_archtype_map[entity];
				std::shared_ptr<Archtype> archtype = AssetManagerSystem::GetResource<Archtype>({ archtype_guid, AssetManagerSystem::ARCHTYPE_GUID });
				//Only serialize archtype guid if archtype is valid, i.e. hasn't been deleted.
				if (archtype)
				{
					entity_save_data[AssetManagerSystem::ARCHTYPE] = archtype_guid.m_Value;
					//Record which components belong to the archtype so they don't get duplicate serialized.
					for (const entt::meta_any& archtype_comp : archtype->component_arr)
					{
						archtype_components.emplace_back(archtype_comp.type());
					}
				}
			}

			/*
			* 	Iterate over all components that this entity has.
				To do so, iterate over all possible component types and see which does the entity have.

				<type_id, sparse_set> where type_id represents the component type.

				Note: Cannot simply iterate over all components for an entity, as components are not
				stored in the entity but rather stored in their own pools based on type.
			*/
			//for (auto&& [comp_type_id, component_type] : entt::resolve())
			//Iterate over all types stored in registry.
			for(auto&& comp_storage : registry.storage())
			{
				//If an entity does not contain this specific component type, continue to the next type.
				if (!comp_storage.second.contains(entity)) continue;
				//Check if it's a registered component
				entt::meta_type component_type = entt::resolve(comp_storage.second.info());
				if (!component_type) continue; //Not a valid registered component.
				//Only serialize components. Registered types which aren't components (like glm::vec3) shouldn't be stored.
				if (component_type.traits<REGISTERED_TYPE>() == REGISTERED_TYPE::NON_COMPONENT) continue;
	
				
				//If it belongs to the archtype, don't save it.
				if (std::find(archtype_components.begin(), archtype_components.end(), component_type) != archtype_components.end())
				{
					continue;
				}
				
				
				//Save the component as a json object.
				nlohmann::json component_save_data{};
				/*
					Component name
					->Name of data member, value
				*/
				entt::meta_any component_data = component_type.func("get"_hs).invoke({}, entity);
				//==Iterate over each data type in the component. Only store data, not functions or ptrs.
				for (auto&& member : component_type.data())
				{
					//Get the data value as meta_any.
					SetJsonWithMetaAny(component_save_data, member.second, component_data.get(member.first));
				}

				//Add the component object as part of the entity json object.
				entity_save_data[component_type.name()] = component_save_data;
			}
			if (entity_save_data.empty())
			{
				LOGD("[GameObjectSystem] No archtypes or components detected for entity, it will not be serialized. Was its components registered?");
				continue;
			}
			//Add the entity object as part of the final json object.
			save_data[std::to_string(id++)] = entity_save_data;
		}

		//Return json as a string object, but prettified.
		return save_data.dump(1, '\t');
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
					entity_id = GameObjectSystem::Instantiate(archtype, archtype_guid);
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
					throw std::runtime_error((std::string("[GameObjectSystem] Emplace function not registered for this component: ") + comp_data.type().name()).c_str());
				}
				entt::meta_any success = func.invoke({}, entity_id, static_cast<const void*>(&comp_data));
				//Duplicate component already present inside.
				if (!success.cast<bool>())
				{
					//Silently give an error. 
					LOGE("[GameObjectSystem] Duplicate component %s already present in Instance, will be ignored", comp_data.type().name());
				}
			}

		}
		//LOGD("Successfully Deserialized %s", json_input.c_str());
	}

	entt::entity GameObjectSystem::Instantiate(std::shared_ptr<Archtype> archtype, const xresource::instance_guid& archtype_instance_guid)
	{
		entt::entity entity = registry.create();

		//Emplace all components inside the entity.
		//Each component must have an "emplace" lambda/function registered.
		for (const entt::meta_any& comp : archtype->component_arr)
		{
			auto func = comp.type().func("emplace"_hs);
			if (!func)
			{
				throw std::runtime_error((std::string("[GameObjectSystem] Emplace function not registered for this component: ") + comp.type().name()).c_str());
			}
			entt::meta_any success = func.invoke({}, entity, static_cast<const void*>(&comp));
			//Duplicate component already present inside.
			if (!success.cast<bool>())
			{
				//Silently give an error. 
				LOGE("[GameObjectSystem] Duplicate component %s already present in Archtype, will be ignored", comp.type().name());
			}
		}
		entity_archtype_map.emplace(entity, archtype_instance_guid);

		return entity;
	}

	/*
			Creates a single entity with the same component data (copied) from archtype
		*/
	entt::entity GameObjectSystem::CreateInstance(std::string archtype_str)
	{
		//Get archtype
		xresource::full_guid archtype_guid = AssetManagerSystem::GetFullGuid(AssetManagerSystem::ARCHTYPE_GUID, archtype_str);
		std::shared_ptr<Archtype> archtype = AssetManagerSystem::GetResource<Archtype>(archtype_guid);
		if (!archtype)
		{
			//Fail silently, in the case where archtype is not loaded and/or is deleted. Instance is loaded without the archtype.
			LOGE("[GameObjectSystem] Deserialization: No archtype loaded for %s, instance cannot be instantiated from it", archtype_str.c_str());
			return registry.create(); //Create and return empty entity object.
		}
		//Create instance from archtype component data.
		return Instantiate(archtype, archtype_guid.m_Instance);
	}
}