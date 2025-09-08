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


namespace EngineFramework
{

	/*
		Controls serialization, deserialization, safe deletion of instances.
		Also controls serialization and deserialization for archtypes.

		Note: The way archtypes are stored/read are different from the way instances are stored/read.
		Under the hood this can be ignored, but it can lead to slow archtype serialization.

		Archtype:
		- Stored individually in each file with their own name.
		- All components of an archtype are iterated over and serialized before going to the next archtype
		--> As different component types are stored in separate pools, to get all components of an archtype all pools have to be iterated over and checked for that entity id.
		--> Thus, this can lead to slower run times with growing number of component types + growing number of archtypes.

		Instance:
		- Stored together in a single level file.
		- First, all instance entity IDs are stored.
		- Next, all components of a specific type are iterated over and stored. They may belong to different entities.
		- This repeats for all component types.

		If archtype serialization performance is a critical issue, store all archtypes in a single file and use the "instance" method (but use names instead of entity id)
	*/
	class GameObjectSystem : public ISystem
	{
	public:
		GameObjectSystem();

		//Return reference to the one and only instance of this system, created by core engine.
		static GameObjectSystem& GetInstance();
	private:
		void Initialize() override;
		void Update(float delta_time) override;

		/*
			Save gameobject archtypes to files using reflection.
			Each archtype is saved to their own respective files with their own respective names, in the folder.
		*/
		void SerializeAllEntityArchtypes(const std::string& folder_path);

		/*
			Read gameobject archtypes from files using reflection.
			Each archtype is read from their own respective files with their own respective names, in the folder.
			After construction, each entity id is stored together with its name inside the archtype entities map.

			Do once at the start of the program. 
		*/
		void DeserializeAllEntityArchtypes(const std::string& folder_path);

		/*
			Save gameobject instances to a specified level file using reflection.
		*/
		void SerializeAllEntityInstances(const std::string& level_file_path);

		/*
			Read gameobject instances from a specified level file using reflection.
			Before this, all archtypes need to be read.
		*/
		void DeSerializeAllEntityInstances(const std::string& level_file_path);
		
		//Keeps track of the one and only instance of this system, created by core engine.
		static inline GameObjectSystem* this_system{};

		//The archetype entities used for instantiating instances. 
		std::unordered_map<std::string, entt::entity> archtype_entities;
	};

	
}




#endif