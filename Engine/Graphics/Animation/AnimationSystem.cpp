#include "pch.hpp"
#include "global.hpp"
#include "Debug.h"
#include "AnimationSystem.h"
#include "GraphicsSystem.h"

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
		glfwTerminate(); 
	}

	void AnimationSystem::Update(float deltaTime) {
		auto view = registry.view<TransformComponent, AnimationComponent>();

		for (auto [entity, transform, anim] : view.each())
		{ 
			// Check if animation exists
			if (!anim.skeleton || !anim.animationPackage) {
				continue;
			}

			// Step 1 — Advance animation time
			anim.player.package = anim.animationPackage.get();
			anim.player.currentAnimIndex = anim.currentClip;
			anim.player.update(deltaTime);

			// Step 2 — Compute world matrix
			glm::mat4 world;
			ComputeWorldMatrix(transform, world);

			// Step 3 — Resize palette buffer if needed
			int boneCount = anim.skeleton->count();
			if (anim.paletteMatrices.size() != (size_t)boneCount) {
				anim.paletteMatrices.resize(boneCount);
			}

			// Step 4 — Build bone palette
			anim.player.computeMatrices(anim.paletteMatrices, world, *anim.skeleton);
		}
	}

	void AnimationSystem::ComputeWorldMatrix(const TransformComponent& transform, glm::mat4& outWorldMatrix){
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