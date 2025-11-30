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
#include <cstddef>

//Entt reflection system
#include <entt/meta/meta.hpp>
#include <entt/meta/resolve.hpp>

//Entt ECS system
#include <entt/entity/registry.hpp>
#include "System_GameObjectSystem.hpp"
#include <entt/core/type_traits.hpp>

//Casting meta data to type.
#include "Archtype.hpp"
#include "ReflectionRegister.hpp"

// Game Object Components
#include "AnimationComponent.h"
#include "TransformComponent.h"
#include "MeshComponent.h"
#include "MaterialComponent.h"
#include "LightComponent.h"
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

	// To get the entity name to be displayed
	std::string GameObjectSystem::GetEntityName(entt::entity entity) {
		if (entity == entt::null || !registry.valid(entity)) {
			return {};
		}
		try
		{
			auto it = entity_archtype_map.find(entity);
			if (it != entity_archtype_map.end()) {
				const xresource::full_guid archtype_guid{ it->second, AssetManagerSystem::ARCHTYPE_GUID };
				std::string name = AssetManagerSystem::GetName(archtype_guid);
				if (!name.empty()) {
					return name;
				}
			}
		}
		catch (std::exception& e)
		{
			UNREFERENCED_PARAMETER(e);
			//Do nothing, fallthrough and return entity id.
		}

		// Fall back for if there is no proper name of the entity
		std::ostringstream label;
		label << "Entity " << static_cast<std::uint32_t>(entt::to_entity(entity));
		return label.str();
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
		std::string start_scene = AssetManagerSystem::GetStringFromFile("Extern/Scenes/M3_Level.json");
		LoadScene(start_scene);
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
		//Get the archtypes stored in asset manager.
		std::vector<std::pair<xresource::full_guid, std::shared_ptr<Archtype>>> archtype_arr = AssetManagerSystem::GetAllResourceOfType<Archtype>(AssetManagerSystem::ARCHTYPE_GUID);
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


	Archtype GameObjectSystem::CreateArchtypeFromInstance(entt::entity entity, const std::string& model_type)
	{
		//Serialize the instance component data into a json string, then construct an archtype from that json.
		//Note: This is similar to SerializeAllEntityInstances().
		if (!registry.valid(entity)) 
		{
			throw std::invalid_argument(std::string("Entity not valid: ") + std::to_string((unsigned long)entity));
		}

		nlohmann::json archtype_data{};
		/*
		* 	Iterate over all components that this entity has.
			To do so, iterate over all possible component types and see which does the entity have.

			<type_id, sparse_set> where type_id represents the component type.

			Note: Cannot simply iterate over all components for an entity, as components are not
			stored in the entity but rather stored in their own pools based on type.
		*/
		//Iterate over all types stored in registry.
		for (auto&& comp_storage : registry.storage())
		{
			//If an entity does not contain this specific component type, continue to the next type.
			if (!comp_storage.second.contains(entity)) continue;
			//Check if it's a registered component
			entt::meta_type component_type = entt::resolve(comp_storage.second.info());
			if (!component_type) continue; //Not a valid registered component.
			//Only serialize components. Registered types which aren't components (like glm::vec3) shouldn't be stored.
			if (component_type.traits<REGISTERED_TYPE>() == REGISTERED_TYPE::NON_COMPONENT) continue;

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
			archtype_data[component_type.name()] = component_save_data;
		}
		archtype_data["model_type"] = model_type;

		return Archtype(archtype_data.dump());
	}

	void GameObjectSystem::AssignArchtypeToInstance(entt::entity entity, xresource::instance_guid archtype_guid)
	{
		entity_archtype_map[entity] = archtype_guid;
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
		// Iterate over all entities in the registry.
		auto &entity_storage = registry.storage<entt::entity>();
		for (entt::entity entity : entity_storage)
		{
			if (!registry.valid(entity)) {
				continue;
			}
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
			save_data[std::to_string(entt::to_entity(entity))] = entity_save_data;
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
			entt::entity entity_id = registry.create(entt::entity(std::stoul(key)));
			if (entity_json.contains("archtype"))
			{
				xresource::instance_guid archtype_guid{ entity_json["archtype"].get<uint64_t>() };
				std::shared_ptr<Archtype> archtype = AssetManagerSystem::GetResource<Archtype>({ archtype_guid, AssetManagerSystem::ARCHTYPE_GUID });
				if (archtype)
				{
					entity_id = GameObjectSystem::Instantiate(archtype, archtype_guid, entity_id);
				}
				else
				{
					//Fail silently, in the case where archtype is not loaded and/or is deleted. Instance is loaded without the archtype.
					LOGE("[GameObjectSystem] Deserialization: No archtype loaded for %" PRIu64 "guid, instance cannot be instantiated from it", archtype_guid.m_Value);
				}
			}
			//Else if there's no archtype, then don't do anything and start off with no components.
			
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

	entt::entity GameObjectSystem::Instantiate(std::shared_ptr<Archtype> archtype, const xresource::instance_guid& archtype_instance_guid, entt::entity entity)
	{
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
		return Instantiate(archtype, archtype_guid.m_Instance, registry.create());
	}

	// For duplication of instances
	// 1. Validates source entity
	// 2. Creates new registry entry and walk through every component to clone the reflected components via registered get/emplace functions
	// Duplicated components must expose get, get_ref, emplace, remove in reflectionregister or wont copy
	entt::entity GameObjectSystem::DuplicateInstance(entt::entity entity)
	{
		if (entity == entt::null || !registry.valid(entity))
		{
			LOGE("[GameObjectSystem] Cannot duplicate invalid entity %" PRIu32, static_cast<uint32_t>(entt::to_entity(entity)));
			return entt::null;
		}

		entt::entity duplicated_entity = registry.create();

		for (auto&& comp_storage : registry.storage())
		{
			auto& storage = comp_storage.second;
			if (!storage.contains(entity)) continue;

			entt::meta_type component_type = entt::resolve(storage.info());
			if (!component_type) continue;
			if (component_type.traits<REGISTERED_TYPE>() == REGISTERED_TYPE::NON_COMPONENT) continue;

			auto get_func = component_type.func("get"_hs);
			auto emplace_func = component_type.func("emplace"_hs);
			if (!get_func || !emplace_func) continue;

			entt::meta_any component_data = get_func.invoke({}, entity);
			if (!component_data) continue;

			entt::meta_any result = emplace_func.invoke({}, duplicated_entity, static_cast<const void*>(&component_data));
			if (result && result.type() == entt::resolve<bool>() && !result.cast<bool>())
			{
				LOGE("[GameObjectSystem] Duplicate component %s already present when duplicating entity %" PRIu32,
					component_type.name(), static_cast<uint32_t>(entt::to_entity(duplicated_entity)));
			}
		}

		auto archtype_entry = entity_archtype_map.find(entity);
		if (archtype_entry != entity_archtype_map.end())
		{
			AssignArchtypeToInstance(duplicated_entity, archtype_entry->second);
		}

		return duplicated_entity;
	}

	/*
			Creates a single entity with the same component data (copied) from archtype,
			and sets its transform.
		*/
	entt::entity GameObjectSystem::CreateInstance(std::string archtype, const TransformComponent& transform)
	{
		entt::entity entity = CreateInstance(archtype);
		registry.emplace_or_replace<TransformComponent>(entity, transform);
		return entity;
	}
}
