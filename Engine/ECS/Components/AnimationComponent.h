/**
 * @file AnimationComponent.h
 * @brief Defines the AnimationComponent structure required for Animation System.
 *
 * This file contains the `AnimationComponent` structure, which allows for a mesh / 3D model which 
 * has animation data to be animated via the Animation System in the EngineFramework.
 *
 * @date Nov 2025
 */
#ifndef ANIM_COMPONENT
#define ANIM_COMPONENT

struct AnimationComponent
{
	// Name of animation file to use
	std::string animationName;

	// Pointer to the skeleton data for the animated model
	EngineFramework::Skeleton* skeleton = nullptr;
	// Pointer to the animation package containing all geminianimations for the model
	EngineFramework::AnimationPackage* animationPackage = nullptr;

	// Animation player to manage current animation state
	EngineFramework::AnimationPlayer player; 
	int currentClip = 0;

	// Palette matrices for skinning
	std::vector<glm::mat4> paletteMatrices; 
	glm::mat4 globalAnimationTransform = glm::mat4(1.0f);

	// Bool to check if loaded
	bool loaded = false;
};
// --- Other Scene Entity Components ---

// Component to tag the primary/active camera for a scene
//struct ActiveCamera {};
#endif