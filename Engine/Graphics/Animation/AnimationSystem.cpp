#include <stdio.h>
#include "pch.hpp"
#include "global.hpp"
#include "Debug.h"
#include "AnimationSystem.h"
#include "GraphicsSystem.h"
#include "AnimationComponent.h"
#include <glm/gtx/string_cast.hpp>

#if defined(DEBUG) && defined(PLATFORM_WINDOWS)
#include "System_LevelEditor.hpp"
class LevelEditor; //forward declaration
#endif

namespace EngineFramework
{
	AnimationSystem::AnimationSystem() {
		AnimationSystemPtr = this;
	}

	AnimationSystem::~AnimationSystem() {
	}

	void AnimationSystem::Update(float deltaTime) {
		auto view = registry.view<TransformComponent, AnimationComponent>();

		for (auto [entity, transform, anim] : view.each())
		{
			// Check if animation was successfully loaded
			if (!anim.loaded) {
				// Get the animation guid
				auto anim_guid = AssetManagerSystem::GetFullGuid(AssetManagerSystem::ANIMATION_GUID, anim.animationName);
				auto bundle = AssetManagerSystem::GetResource<animImport>(anim_guid);

				// Check the bundle is valid
				if (!bundle) {
					// Asset not found
					continue;
				}

				// Assign it back to current component if it is valid
				anim.skeleton = bundle->skeleton.get();
				anim.animationPackage = bundle->animations.get();

				// Tell and set the component to have initialized
				anim.loaded = true;

				float scaleFactor = 10.0f;
				glm::mat4 scaleF = glm::scale(glm::mat4(1.0f), glm::vec3(scaleFactor));
				anim.globalAnimationTransform = scaleF;				anim.currentClip = 0;
				anim.player.currentAnimIndex = anim.currentClip;
				anim.player.package = anim.animationPackage;
				anim.player.time = 0.0f;

				anim.paletteMatrices.resize(anim.skeleton->count());
			}

			// Check if animation exists
			if (!anim.skeleton || !anim.animationPackage) {
				continue;
			}

			// Step 1 � Advance animation time
			anim.player.update(deltaTime);

			// Step 2 � Compute world matrix
			glm::mat4 world;
			ComputeWorldMatrix(transform, world);

			// Step 3 � Resize palette buffer if needed
			int boneCount = anim.skeleton->count();
			if (anim.paletteMatrices.size() != (size_t)boneCount) {
				anim.paletteMatrices.resize(boneCount);
			}

			// Step 4 � Build bone palette
			anim.player.computeMatrices(anim.paletteMatrices, anim.globalAnimationTransform, *anim.skeleton);

			// Find bone indices
			int root = -1, hip = -1, knee = -1, ankle = -1;
			for (int i = 0; i < anim.skeleton->bones.size(); ++i) {
				if (anim.skeleton->bones[i].name == "Root") root = i;
				if (anim.skeleton->bones[i].name == "Hip") hip = i;
				if (anim.skeleton->bones[i].name == "Knee") knee = i;
				if (anim.skeleton->bones[i].name == "Ankle") ankle = i;
			}

			// Log matrices
			if (root != -1) {
				printf("Root: %s\n", glm::to_string(anim.paletteMatrices[root]).c_str());
			}
			if (hip != -1) {
				printf("Hip: %s\n", glm::to_string(anim.paletteMatrices[hip]).c_str());
			}
			if (knee != -1) {
				printf("Knee: %s\n", glm::to_string(anim.paletteMatrices[knee]).c_str());
			}
			if (ankle != -1) {
				printf("Ankle: %s\n", glm::to_string(anim.paletteMatrices[ankle]).c_str());
			}
		}
	}

	void AnimationSystem::ComputeWorldMatrix(const TransformComponent& transform, glm::mat4& outWorldMatrix) {
		// Set to identity
		glm::mat4 out(1.0);

		// Add translation of position
		out = glm::translate(out, transform.position);

		// Add all axis of rotation
		out = glm::rotate(out, glm::radians(transform.rotation.x), glm::vec3(1, 0, 0));
		out = glm::rotate(out, glm::radians(transform.rotation.y), glm::vec3(0, 1, 0));
		out = glm::rotate(out, glm::radians(transform.rotation.z), glm::vec3(0, 0, 1));

		// Finally add the scale and pass it back to World matrix
		outWorldMatrix = glm::scale(out, transform.scale);
	}

}