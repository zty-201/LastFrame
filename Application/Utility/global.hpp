/******************************************************************************/
/*!
\file		global.hpp
\author 	Joel Lee Jie
\par    	joeljie.lee\@digipen.edu
\brief  	Contains frequently used macros, preprocessor definitions, constants and variables. Global definitions.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the prior
written consent of DigiPen Institute of Technology is prohibited.
*/
/******************************************************************************/
#ifndef GLOBAL_HPP
#define GLOBAL_HPP
#include "entt/entity/registry.hpp"
#include "entt/meta/meta.hpp"


//Used to denote a variable is unused.
#define UNREFERENCED_PARAMETER(P) (P)
//Defines max delta time so the game doesn't have too big of a delta time value, like if the user shifts the window. Arbitrary value.
#define MAX_DELTA_TIME 0.1f
#define PI 3.14159265358979323846f


//Global ecs system.
extern entt::registry registry;

//TODO: replace with config file.
#define ARCHTYPE_FOLDER_PATH "Extern/Archtypes"

#endif