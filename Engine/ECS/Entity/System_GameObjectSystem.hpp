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

namespace EngineFramework
{

	/*
		Controls serialization, deserialization, safe deletion of instances.
		Also controls serialization and deserialization for archtypes.

		Archtype:
		- Stored individually in each file with their own name.
		- Not part of the game (not in registry), only job is to hold component data and create entt::entity instances copying the stored component data.
		- component data will not have any allocated data, it will only have data that can be deserialized from file. ensure component destructors account for this. 

		Instance:
		- Stored together in a single level file.
		- First, all instance entity IDs are stored.
		- Next, all components of a specific type are iterated over and stored. They may belong to different entities.
		- This repeats for all component types.


		==Example Deserialization/Serialization of Archtypes==
		//==Testing of deserialization + serialization of one archtype
		/*LOGD("%s", "Testing deserialization of archetype");
		std::vector<unsigned char> data = EngineFramework::AssetManagerSystem::GetDataFromFile(std::filesystem::path("Extern/Archtypes/testclass.json"));
		std::string test_string{ data.begin(), data.end() };
		std::vector<std::pair<std::string, std::string>> test_arr{ std::make_pair(std::string("testclass"), test_string) };
		DeserializeAllEntityArchtypes(test_arr);
		archtype_arr.find("testclass")->second.Instantiate();
		archtype_arr.find("testclass")->second.Instantiate();
		for (auto entity : registry.view<TODELETE>())
		{
			TODELETE& pos = registry.view<TODELETE>().get<TODELETE>(entity);
			LOGD("%s", pos.c.c_str());
		}
		auto archtype_json_arr = SerializeAllEntityArchtypes();
		LOGD("%s", "testing");
		
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
		- Note: doing archtype_arr["name"] will throw an error, as [] is both a find() and a constructor. Constructing however is not allowed if no parameter is passed in. use .find() instead.

		Changing Archtype Name
		- Access archtype_map using .find()
		- Copy archtype, then delete pair from map
		- Reinsert archtype into map with new name.
		- May also need to change map<entityid, archtype_name_string> to the changed archtype_name_string for serialization.
	*/
	class GameObjectSystem : public ISystem
	{
	public:
		GameObjectSystem();

		//Return reference to the one and only instance of this system, created by core engine.
		static GameObjectSystem& GetInstance();

		//Quick Call function to load scene and destroy current scene.
		void LoadScene(const std::string& json_input);
		
	private:
		void Initialize() override;
		void Update(float delta_time) override;
		
		//Clears all entities in the scene
		void ClearAllInstances();
		
		/*
		 	Write to json strings using reflection.
			Returns an array of <archetype name, json string> where each pair represents the archetype name and the json string to save to
			their file. Use the archetype name as the file name when saving.
		*/
		std::vector<std::pair<std::string, std::string>> SerializeAllEntityArchtypes();

		/*
			Read gameobject archtypes from json strings using reflection.
			Each archtype is read as a pair of <archetype name, json string> stored in the archetype array passed to this function.
			Ensure the archetype name is completely read (from the file name) and accounts for spaces.
			After construction, each entity id (representing an archetype) is stored together with its name inside the archtype entities map.

			Do once at the start of the program. 
		*/
		void DeserializeAllEntityArchtypes(const std::vector<std::pair<std::string, std::string>>& json_input);

		/*
			Write gameobject instances to a json string using reflection.
			Returns a json string that can be written to a scene file.
		*/
		std::string SerializeAllEntityInstances();

		/*
			Read gameobject instances from json string using reflection.
			The json string represents a json string read from a scene file.
			After construction, entities are stored inside entt::registry.

			Ensure all archetypes are deserialized before calling this.
		*/
		void DeSerializeAllEntityInstances(const std::string& json_input);
		
	
		/*
			Creates a single entity with the same component data (copied) from archtype
		*/
		entt::entity CreateInstance(std::string archtype);

		//The archetypes used for instantiating entity instances
		std::unordered_map<std::string, Archtype> archtype_arr;

		//Keeps track of the one and only instance of this system, created by core engine.
		static inline GameObjectSystem* this_system{};
	};

	
}




#endif