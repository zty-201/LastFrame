/*
*****************************************************************/
/*!
\file   Input.hpp
\author Tang Yeun, Dean
\par    y.tang\@digipen.edu
\date   September 2025
\brief
This file contains the declarations of the functions that are needed for
handling the inputs in the system. 
Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* *******************************************************************/
#ifndef INPUT_MANAGER_H
#define INPUT_MANAGER_H

#include <unordered_map>
#include "ISystem.hpp"

enum class KeyState {
	None,		//Default state, no interaction with key
	Pressed,	//Key pressed this frame
	Down,		//Key held
	Released	//Key released this frame
};

class InputManager : public EngineFramework::ISystem
{
public:
	void Update(float delta_time) override;
	void PostUpdate(float delta_time) override;
	static bool GetKey(int key);
	static bool GetKeyDown(int key);
	static bool GetKeyUp(int key);

private:
	static std::unordered_map<int, KeyState> keyStates;
};

#endif