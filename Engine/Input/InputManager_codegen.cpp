#include "InputManager.hpp"
namespace EngineFramework
{
	void InputManager::initialise_keymapping() {
		key_mapping[Key_Actions::JUMP] = 32;
		key_mapping[Key_Actions::MOVE_BACK] = 75;
		key_mapping[Key_Actions::MOVE_FORWARD] = 73;
		key_mapping[Key_Actions::MOVE_LEFT] = 74;
		key_mapping[Key_Actions::MOVE_RIGHT] = 76;
		key_mapping[Key_Actions::OPEN_EDITOR] = 57;
	}
}
