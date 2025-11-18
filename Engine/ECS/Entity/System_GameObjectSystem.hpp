/*
		Controls serialization, deserialization, safe deletion of instances.
		Also controls serialization and deserialization for archtypes.
		
		Uses Entt as the backbone.
*/
#ifndef SYSTEM_GAME_OBJECT_LOADER_HPP
#define SYSTEM_GAME_OBJECT_LOADER_HPP
#include <unordered_map>
#include <string>
#include <sstream>
#include "ISystem.hpp"
#include "entt/entity/registry.hpp"
#include "Archtype.hpp"
#include "TransformComponent.h"
namespace EngineFramework
{
#if defined(PLATFORM_WINDOWS) && defined(_DEBUG)

	class Save; //forward declaration
#endif
	
	

	/*
		Controls serialization, deserialization, safe deletion of instances.
		Also controls serialization for archtypes.

		Archtype:
		- Stored individually in each file with their own name.
		- Not part of the game (not in registry), only job is to hold component data that can be used to create entities by copying the component data stored.
		- component data will not have any allocated data, it will only have data that can be deserialized from file. ensure component destructors account for this.
		- Once deserialized, will be stored in assetmanager. Can be accessed indirectly via CreateInstance() 

		Instance:
		- Stored together in a single level file.
		- First, all instances are stored with incremental ids. These ids (currently) do not represent entity id, they're just used for differentiating instances in the json file.
		- Each instance can have an optional "archtype" : "archtype_name" that it can be based off.
		- Next, all components of a specific type are iterated over and stored. They may belong to different entities.
		- This repeats for all component types.

		Renaming of Archtypes:
		- Similar to renaming of assets in assetmanager (since an archtype is also an asset). 
		- Ensure the guid_to_name map in assetmanager is changed appropriately.
		
		Serialization
		- string is read from archtype files, and each <file_name_string, json_string> is added to a vector   
		- Read file_name using std::filsystem::path directory_iterator, to include whitespaces in the name. (note that file_name is stem(), i.e. without slashes and without extension)(note that file_name is stem(), i.e. without slashes and without extension)
		- DO NOT USE IFSTREAM. Call AssetManagerSystem::GetDataFromFile instead, and convert vector<unsigned char> to std::string.
		- vector of archtype jsons is passed to SerializeAllEntityArchtypes.

		Deserialization
		- When called, archtypes in archtype_arr will write their json string to <archtype_name_string, json_string>.
		- the strings from all archtypes are added to a vector and returned by DeserializeAllEntityArchtypes
		- Just iterate over each pair <archtype_name_string, json_string> in the vector, and write to separate files using archtype_name as the file name.

		Instantiation
		- Use CreateInstance("archtype_name") where the archtype name is the file name.

		entt::view for iterating
		- all entities
		- all entities with so and so component
		entt::storage for iterating over all components of a specific type.

		Deletion of instance
		- Ensure the respective entry in entity_archtype_map is deleted, so it won't store unnecessary entity.

	*/
	class GameObjectSystem : public ISystem
	{
	public:
		GameObjectSystem();

		//Return reference to the one and only instance of this system, created by core engine.
		static GameObjectSystem& GetInstance();

		static std::string GetEntityName(entt::entity entity);

		//Quick Call function to load scene and destroy current scene.
		void LoadScene(const std::string& json_input);
		
		/*
			Creates a single entity with the same component data (copied) from archtype
		*/
		entt::entity CreateInstance(std::string archtype);

		/*
			Creates a single entity with the same component data (copied) from archtype,
			and sets its transform
		*/
		entt::entity CreateInstance(std::string archtype, const TransformComponent& transform);

		// Getter for registry
		static entt::registry& GetRegistry() { return registry; }
		//Clears all entities in the scene
		void ClearAllInstances();
		/*
			Write gameobject instances to a json string using reflection.
			Returns a json string that can be written to a scene file.
		*/
		std::string SerializeAllEntityInstances();

	private:
		void Initialize() override;
		void Update(float delta_time) override;
		
		
		
		/*
		 	Write to json strings using reflection.
			Returns an array of <archetype name, json string> where each pair represents the archetype name and the json string to save to
			their file. Use the archetype name as the file name when saving.
		*/
		std::vector<std::pair<std::string, std::string>> SerializeAllEntityArchtypes();

		

		/*
			Read gameobject instances from json string using reflection.
			The json string represents a json string read from a scene file.
			After construction, entities are stored inside entt::registry.

			Ensure all archetypes are deserialized before calling this.
		*/
		void DeSerializeAllEntityInstances(const std::string& json_input);
		
		/*
			Creates an entt::entity instance with an archtype.
			entity_id is the id to instantiate with.
		*/
		static entt::entity Instantiate(std::shared_ptr<Archtype> archtype, const xresource::instance_guid& archtype_instance_guid, entt::entity entity);

		//<entity_id, archtype_instance_guid> mapping, used when serializing instances back to scene file.
		static inline std::unordered_map<entt::entity, xresource::instance_guid> entity_archtype_map{};

		//Keeps track of the one and only instance of this system, created by core engine.
		static inline GameObjectSystem* this_system{};
#if defined(PLATFORM_WINDOWS) && defined(_DEBUG)

		friend Save;
#endif
		
	};

	
}




#endif