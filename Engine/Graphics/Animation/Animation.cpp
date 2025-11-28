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

    void AnimationPlayer::computeMatrices(std::vector<glm::mat4>& finalMatrices,
        const glm::mat4& /*worldTransform*/,
        const Skeleton& skeleton) const
    {
        // DEBUG: see whether we even have animations
        size_t animCount = package ? package->animations.size() : 0;
        LOGI("computeMatrices: package=%p, animCount=%zu, currentAnimIndex=%d",
            (void*)package, animCount, currentAnimIndex);

        // If no package or no clips, DON'T return with zeros; use bind pose.
        if (!package || package->animations.empty()) {
            LOGE("computeMatrices early-exit: no animation package or no clips, using bind pose.");
            finalMatrices.assign(skeleton.count(), glm::mat4(1.0f));
            for (int i = 0; i < skeleton.count(); ++i) {
                finalMatrices[i] = skeleton.bones[i].invBind;
            }
            return;
        }

        const Animation& anim = package->animations[currentAnimIndex];

        finalMatrices.assign(skeleton.count(), glm::mat4(1.0f));

        // Fallback: animation has no keyframes -> bind pose
        if (anim.boneKeyframes.empty() || anim.numFrames() <= 0) {
            LOGE("computeMatrices: clip has no keyframes or frames, using bind pose.");
            for (int i = 0; i < skeleton.count(); ++i) {
                finalMatrices[i] = skeleton.bones[i].invBind;
            }
            return;
        }

        // 1) frame indices
        float frameTime = time * anim.fps;
        int f0 = static_cast<int>(frameTime);
        int f1 = (f0 + 1) % anim.numFrames();
        float blend = frameTime - f0;

        std::vector<glm::mat4> locals(skeleton.count());
        std::vector<glm::mat4> globals(skeleton.count());

        // 2) build local transforms
        for (int i = 0; i < skeleton.count(); ++i) {
            glm::mat4 local = glm::mat4(1.0f);

            if (i < static_cast<int>(anim.boneKeyframes.size())) {
                const BoneKeyframes& kf = anim.boneKeyframes[i];

                if (!kf.frames.empty()) {
                    int maxFrameIndex = static_cast<int>(kf.frames.size()) - 1;
                    int cf0 = glm::clamp(f0, 0, maxFrameIndex);
                    int cf1 = glm::clamp(f1, 0, maxFrameIndex);

                    const keyFrame& a = kf.frames[cf0];
                    const keyFrame& b = kf.frames[cf1];
                    local = Blend(a, b, blend).toMatrix();
                }
            }

            locals[i] = local;
        }

        // 3) concat to globals (no world baked in; u_Model handles that)
        for (int i = 0; i < skeleton.count(); ++i) {
            int parent = skeleton.bones[i].parentIndex;
            if (parent >= 0) {
                globals[i] = globals[parent] * locals[i];
            }
            else {
                globals[i] = locals[i];
            }
        }

        // 4) final skinning matrices
        for (int i = 0; i < skeleton.count(); ++i) {
            finalMatrices[i] = globals[i] * skeleton.bones[i].invBind;
        }

        // DEBUG: print bone 0 result
        const glm::mat4& m0 = finalMatrices[0];
        LOGI("computeMatrices result bone[0]:\n"
            "[%f %f %f %f]\n"
            "[%f %f %f %f]\n"
            "[%f %f %f %f]\n"
            "[%f %f %f %f]",
            m0[0][0], m0[0][1], m0[0][2], m0[0][3],
            m0[1][0], m0[1][1], m0[1][2], m0[1][3],
            m0[2][0], m0[2][1], m0[2][2], m0[2][3],
            m0[3][0], m0[3][1], m0[3][2], m0[3][3]);
    }
}