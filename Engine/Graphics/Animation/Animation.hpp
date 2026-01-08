#ifndef ANIMATION_COMPONENT_HPP
#define ANIMATION_COMPONENT_HPP
#include "ofbx.h"

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

	inline glm::quat ConvertEuler(const glm::vec3& eulerDeg, ofbx::RotationOrder order)
	{
		glm::vec3 rad = glm::radians(eulerDeg);

		glm::quat qx = glm::angleAxis(rad.x, glm::vec3(1, 0, 0));
		glm::quat qy = glm::angleAxis(rad.y, glm::vec3(0, 1, 0));
		glm::quat qz = glm::angleAxis(rad.z, glm::vec3(0, 0, 1));

		switch (order)
		{
		case ofbx::RotationOrder::EULER_XYZ: return qx * qy * qz;
		case ofbx::RotationOrder::EULER_XZY: return qx * qz * qy;
		case ofbx::RotationOrder::EULER_YXZ: return qy * qx * qz;
		case ofbx::RotationOrder::EULER_YZX: return qy * qz * qx;
		case ofbx::RotationOrder::EULER_ZXY: return qz * qx * qy;
		case ofbx::RotationOrder::EULER_ZYX: return qz * qy * qx;
		default: return qx * qy * qz;
		}
	}

	struct keyFrame {
		glm::vec3 scale = glm::vec3(1.0f);
		glm::quat rotation = glm::quat(1, 0, 0, 0); // identity
		glm::vec3 position = glm::vec3(0.0f);

		glm::vec3 preRotDeg;
		glm::vec3 postRotDeg;
		ofbx::RotationOrder rotOrder;

		glm::mat4 toMatrix() const
		{
			// Pre/post rotation quats
			glm::quat qPre = ConvertEuler(preRotDeg, rotOrder);
			glm::quat qPost = ConvertEuler(postRotDeg, rotOrder);

			// Animated rotation is already stored as a quat
			glm::quat qAnim = rotation;

			// FBX composition: R = Rpost * Ranim * Rpre
			glm::quat qFinal = qPost * qAnim * qPre;

			glm::mat4 T = glm::translate(glm::mat4(1.0f), position);
			glm::mat4 R = glm::mat4_cast(qFinal);
			glm::mat4 S = glm::scale(glm::mat4(1.0f), scale);

			// local = T * R * S
			return T * R * S;
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

		const int FindClip(const std::string& n) const {
			for (int i = 0; i < (int)animations.size(); i++) {
				if (animations[i].name == n) return i;
			}
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
		void logBoneTransforms(const Skeleton& skeleton, const std::vector<glm::mat4>& globals) const;
	};

	struct SkinnedVertex {
		glm::vec3 position;
		glm::vec3 normal;
		glm::vec3 tangent;
		glm::vec2 uv;
		std::uint8_t weights[4];    // sum ~ 255
		std::uint8_t boneIndices[4];// indices into palette
	};

	extern std::unordered_map<std::string, int> g_OFBXBoneIndexByName;
}

#endif