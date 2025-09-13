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
#include "System_LevelEditor.hpp"

//Casting meta data to type.
#include "ReflectionRegister.hpp"

#include "LFeature.hpp"

namespace EngineFramework
{
	LFeature::LFeature()
	{
		
	}

	void LFeature::Initialize()
	{
		std::cout << "level editor Lfeature ONLINE!\n";
	}

	void LFeature::Run()
	{

	}

	void LFeature::Close()
	{


	}

}