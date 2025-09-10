/*
*****************************************************************/
/*!
\file   InputManager.cpp
\author Tang Yeun, Dean
\par    y.tang\@digipen.edu
\date   September 2025
\brief
This file contains the definitions of the functions that are needed for
handling the inputs in the system.
Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* *******************************************************************/

#include "InputManager.hpp"
#include "global.hpp"
void InputManager::Update(float delta_time)
{
    UNREFERENCED_PARAMETER(delta_time);
}

void InputManager::PostUpdate(float delta_time)
{
    UNREFERENCED_PARAMETER(delta_time);
}

bool InputManager::GetKey(int key)
{
    UNREFERENCED_PARAMETER(key);
    return false;
}

bool InputManager::GetKeyDown(int key)
{
    UNREFERENCED_PARAMETER(key);
    return false;
}

bool InputManager::GetKeyUp(int key)
{
    UNREFERENCED_PARAMETER(key);
    return false;
}
