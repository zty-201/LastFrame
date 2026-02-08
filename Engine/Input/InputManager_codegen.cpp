#include "InputManager.hpp"
namespace EngineFramework
{
	void InputManager::initialise_keymapping() {
		key_mapping[Key_Actions::FLASHLIGHT] = 81;
		key_mapping[Key_Actions::INTERACT] = 69;
		key_mapping[Key_Actions::INVENTORY] = 73;
		key_mapping[Key_Actions::JUMP] = 32;
		key_mapping[Key_Actions::MOVE_BACK] = 83;
		key_mapping[Key_Actions::MOVE_FORWARD] = 87;
		key_mapping[Key_Actions::MOVE_LEFT] = 65;
		key_mapping[Key_Actions::MOVE_RIGHT] = 68;
		key_mapping[Key_Actions::OPEN_EDITOR] = 57;
		key_mapping[Key_Actions::PAUSE] = 256;
	}
}
