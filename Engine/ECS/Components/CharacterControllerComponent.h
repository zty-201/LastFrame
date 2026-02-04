#pragma once
#include "glm/glm.hpp"

#ifndef CHARACTER_CONTROLLER_COMPONENT
struct CharacterControllerComponent
{
	//Shape Dimensions
	float height = 1.75f;
	float radius = 0.4f;
	float mass = 70.f;

	//Movement Settings
	float maxSpeed = 6.f;
	float jumpStrength = 10.f;

	//Jolt Physics Limits
	float maxSlopeAngle = 50.f;
	float stepHeight = 0.3f;

	//Runtime State
	glm::vec3 controlVelocity = { 0.f, 0.f, 0.f };

	bool isGrounded = false;
	glm::vec3 groundNormal = { 0.f, 1.f, 0.f };

	//Internal Data
	void* runtimeChar = nullptr;
};
#endif
