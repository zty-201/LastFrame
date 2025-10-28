#include "pch.hpp"
#include "Animation.hpp"

namespace EngineFramework
{
	// Small helper function
	static inline int clampInt(int v, int low, int high) {
		return std::max(low, std::min(v, high));
	}

	static keyFrame Blend(const keyFrame& a, const keyFrame& b, float t)
	{
		keyFrame r;
		r.scale = glm::mix(a.scale, b.scale, t);
		r.rotation = glm::normalize(glm::slerp(a.rotation, b.rotation, t));
		r.position = glm::mix(a.position, b.position, t);
		return r;
	}

	void AnimationPlayer::update(float dt) {
		// If Animation exists
		if (!package) return;
		// Else if invalid index is given
		if (currentAnimIndex < 0 || currentAnimIndex >= (int)package->animations.size()) return;

		// Run the animation
		const Animation& anim = package->animations[currentAnimIndex];

		// Validate if the clip has ended
		if (anim.duration <= 0.0f) return;

		time += dt;

		// Ensure proper looping
		while (time >= anim.duration) time -= anim.duration;
		while (time < 0.0f) time += anim.duration;
	}

	void AnimationPlayer::computeMatrices(std::vector<glm::mat4>& finalMatrices, const glm::mat4& worldTransform, const Skeleton& skeleton) const {
		// Check if animation exists
		if (!package || package->animations.empty()) return;

		// Check current animation data
		const Animation& anim = package->animations[currentAnimIndex];
		finalMatrices.assign(skeleton.count(), glm::mat4(1.0f));

		// Fallback if no animations are assigned to it, this just applies the default pose
		if (anim.boneKeyframes.empty() || anim.numFrames() <= 0) {
			for (int i = 0; i < skeleton.count(); i++) {
				finalMatrices[i] = worldTransform; // if you want world baked in; often you keep world separate
				finalMatrices[i] *= skeleton.bones[i].invBind;
			}

			return;
		}

		// Convert time and obtain frame indices along with blend alpha
		float frameTime = time * anim.fps;
		int frame0 = static_cast<int>(frameTime);
		int frame1 = (frame0 + 1) % anim.numFrames();
		float blend = frameTime - frame0;

		// Build local matrices to blend frame0 and frame1
		std::vector<glm::mat4> locals(skeleton.count());
		locals.reserve(skeleton.count());

		// Loop through and get the blending from frame0 to frame1 of each bones
		for (int i = 0; i < skeleton.count(); ++i) {
			const BoneKeyframes& kf = anim.boneKeyframes[i];
			const keyFrame& a = kf.frames[frame0];   // transform at integer frame f0
			const keyFrame& b = kf.frames[frame1];   // transform at next frame f1
			locals[i] = Blend(a, b, blend).toMatrix();  // obtain the transform of current frame
		}

		// Concat matrices to global 
		std::vector<glm::mat4> globals(skeleton.count());
		for (int i = 0; i < skeleton.count(); ++i) {
			int parent = skeleton.bones[i].parentIndex;

			// Check if parent is the root bone else we apply the parent's global matrix
			globals[i] = (parent >= 0) ? globals[parent] * locals[i]
				: worldTransform * locals[i];
		}

		// For skinning 
		for (int i = 0; i < skeleton.count(); ++i) {
			finalMatrices[i] = globals[i] * skeleton.bones[i].invBind;
		}
	}

	

}