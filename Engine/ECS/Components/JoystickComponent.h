#ifndef JOYSTICK_COMPONENT
#define JOYSTICK_COMPONENT

#include <glm/glm.hpp>

struct JoystickComponent {
	float maxRadius = 100.f;
	glm::vec2 baseCenter = { 0.f, 0.f };
	//State tracking
	bool isBeingDragged = false;
	//Final output values (-1.0 to 1.0)
	float outputX = 0.f;
	float outputY = 0.f;

	std::string knobTargetName;

	entt::entity knobEntity{ entt::null };
};

#endif