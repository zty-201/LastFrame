/*
*****************************************************************/
/*!
\file   Script.cpp
\author Teng Jun Rong Jerron
\par    t.junrongjerron@digipen.edu
\date   September 2025
\brief
Multi-script version: allows multiple Lua scripts per entity.
Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* *******************************************************************/

#include "Script.hpp"
#include "Scripting/ScriptAsset.hpp"
#include "AssetManager.hpp"
#include <glm/geometric.hpp> 
#include <iostream>

#if defined(ENGINE_ENABLE_LUA)
#include <mutex>
#include <unordered_set>
#endif

//For camera access in scripts
#include "InputManager.hpp"
#include "pch.hpp"
//File management, JSON file parser/writer
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <cctype>
//Entt reflection system
#include <entt/meta/meta.hpp>
#include <entt/meta/resolve.hpp>
//Entt ECS system
#include <entt/entity/registry.hpp>
#include "System_LevelEditor.hpp"

//Casting meta data to type.
#include "ReflectionRegister.hpp"
#include "KeyMapping.hpp"

namespace Engine {



    void ScriptSystem::Init_Keyaction() {

#if defined(ENGINE_ENABLE_LUA)
        // ===============================================================
            // KeyActions bindings
            // ===============================================================

        sol::table keyActions = GetLuaState().create_named_table("Key_Actions");

        int count = 0;

        if (EngineFramework::InputManager::GetInstance().key_mapping.empty() || EngineFramework::InputManager::GetInstance().key_mapping_data.empty())return;


        for (const auto& [action, keycode] : EngineFramework::InputManager::GetInstance().key_mapping_data)
        {
            for (const auto& [action2, keycode2] : EngineFramework::InputManager::GetInstance().key_mapping)
            {
                if (keycode == keycode2) {
                    keyActions[action] = (int)action2;
                    count++; //must be 9
                    break;
                }
            }
        }

        //std::cout << "There is : " << count << std::endl;


        /*keyActions["FLASHLIGHT"] = (int)EngineFramework::Key_Actions::FLASHLIGHT;
        keyActions["INTERACT"] = (int)EngineFramework::Key_Actions::INTERACT;
        keyActions["INVENTORY"] = (int)EngineFramework::Key_Actions::INVENTORY;
        keyActions["JUMP"] = (int)EngineFramework::Key_Actions::JUMP;
        keyActions["MOVE_BACK"] = (int)EngineFramework::Key_Actions::MOVE_BACK;
        keyActions["MOVE_FORWARD"] = (int)EngineFramework::Key_Actions::MOVE_FORWARD;
        keyActions["MOVE_LEFT"] = (int)EngineFramework::Key_Actions::MOVE_LEFT;
        keyActions["MOVE_RIGHT"] = (int)EngineFramework::Key_Actions::MOVE_RIGHT;
        keyActions["OPEN_EDITOR"] = (int)EngineFramework::Key_Actions::OPEN_EDITOR;*/

#endif

        

    }

    

} // namespace Engine
