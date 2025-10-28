#ifndef ANIMATION_COMPONENT_HPP
#define ANIMATION_COMPONENT_HPP

namespace EngineFramework
{
	struct Bone {
		std::string name;   // "RightArm", etc.
		glm::mat4   invBind = glm::mat4(1.0f);   // inverse bind (global^-1 at bind pose)
		glm::mat4   neutralPose = glm::mat4(1.0f); // optional: bind local (for reference)
		int         parentIndex = -1;            // -1 for root; parents come first
	};

	struct Skeleton {
		std::vector<Bone> bones; // parent before children!
		int count() const { return (int)bones.size(); }
	};

	struct keyFrame {
		glm::vec3 scale = glm::vec3(1.0f);
		glm::quat rotation = glm::quat(1, 0, 0, 0); // identity
		glm::vec3 position = glm::vec3(0.0f);
		glm::mat4 toMatrix() const {
			glm::mat4 T(1.0f);
			T = glm::translate(T, position);
			T *= glm::mat4_cast(rotation);
			T = glm::scale(T, scale);
			return T;
		}
	};

	struct BoneKeyframes {
		std::vector<keyFrame> frames; // size = numFrames
	};

	struct Animation {
		std::string name;
		std::uint64_t guid = 0;
		int fps = 60;     // frames per second
		float duration = 0;  // seconds
		std::vector<BoneKeyframes> boneKeyframes; // size == skeleton.bones.size()
		int numFrames() const {
			return boneKeyframes.empty() ? 0 : (int)boneKeyframes[0].frames.size();
		}
	};

	struct AnimationPackage {
		std::vector<Animation> animations;
		const Animation* byName(const std::string& n) const {
			for (auto& a : animations) if (a.name == n) return &a;
			return nullptr;
		}
	};

	struct AnimationPlayer {
		const AnimationPackage* package = nullptr;
		int   currentAnimIndex = 0;
		float time = 0.0f; // seconds

		void play(int index) { currentAnimIndex = index; time = 0.0f; }
		void play(const AnimationPackage& p, int index) { package = &p; play(index); }
		void update(float dt); // loops automatically

		// Fills finalMatrices with global*invBind palette (for the GPU)
		void computeMatrices(std::vector<glm::mat4>& finalMatrices, const glm::mat4& worldTransform, const Skeleton& skeleton) const;
	};

	struct SkinnedVertex {
		glm::vec3 position;
		glm::vec3 normal;
		glm::vec3 tangent;
		glm::vec2 uv;
		std::uint8_t weights[4];    // sum ~ 255
		std::uint8_t boneIndices[4];// indices into palette
	};

}

#endif