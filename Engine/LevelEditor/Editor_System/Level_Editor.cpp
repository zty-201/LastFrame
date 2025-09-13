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
#include "Level_Editor.hpp"

//Casting meta data to type.
#include "ReflectionRegister.hpp"

namespace EngineFramework
{

	LevelEditor::LevelEditor()
	{
		this_system = this;
		is_enabled = true; //by default it should be set to false

		to_open = false;
		to_close = false;

		std::cout << "LevelEditor() is formed\n";
	}
	
	void LevelEditor::Initialize()
	{
		std::cout << "level editor app is initialised\n";
	}

	void LevelEditor::Open()
	{
		to_open = false; //after open, set this to false.
		std::cout << "app opened\n"; //okay good check
	}

	void LevelEditor::Run()
	{		
		//std::cout << "app running!!!!!!!!!!\n"; //okay good check
	}

	void LevelEditor::Close()
	{

		to_close = false; //after closing, set this to false.
	}

	void LevelEditor::Clean() {

		//when the game is exited, we should clean this up to prevent mem leak
	}

	void LevelEditor::Toggle() {

		if (is_enabled) {
			//if is_enabled = true, then we toggle, so we are closing now
			to_close = true;
		}
		else {
			//if is_enabled = false, then we toggle, so we are opening now
			to_open = true;
		}

		is_enabled = !is_enabled;
	}

}