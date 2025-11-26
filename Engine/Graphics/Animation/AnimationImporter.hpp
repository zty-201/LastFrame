#ifndef ANIMATION_IMPORTER_HPP
#define ANIMATION_IMPORTER_HPP

#include "ofbx.h"

namespace EngineFramework
{
	// Helper functions for FBX processing
	inline std::vector<uint8_t> ReadFBXFile(const std::string& path);

	// Sampling struct
	struct TRSSample {
		glm::vec3 T = { 0.f, 0.f, 0.f };
		glm::vec3 Rdeg = { 0.f, 0.f, 0.f };
		glm::vec3 S = { 1,1,1 };
		bool hasT = false;
		bool hasR = false;
		bool hasS = false;
	};


	// Convert FBX vec3 to glm vec3
	inline glm::vec3 vec3Converter(const ofbx::Vec3& v) {
		return glm::vec3(static_cast<float>(v.x), static_cast<float>(v.y), static_cast<float>(v.z));
	}

	// Convert FBX matrix to glm mat4
	inline glm::mat4 mat4Converter(const ofbx::DMatrix& m) {
		glm::mat4 result;
		for (int row = 0; row < 4; row++) {
			for (int col = 0; col < 4; col++) {
				result[row][col] = static_cast<float>(m.m[row + col*4]);
			}
		}
		return result;
	}

	// Converting Euler angles in degrees (XYZ order) to quaternion
	inline glm::quat EulerDegreesXYZtoQuat(const glm::vec3& deg) {
		// FBX stores Euler angles in degrees; most rigs use XYZ or ZYX; we’ll assume XYZ.
		glm::vec3 rad = glm::radians(deg);
		// glm::quat( pitch(y), yaw(z), roll(x) ) has various conventions; glm’s constructor from euler uses ZYX.
		// To be explicit, use angleAxis chain:
		glm::quat qx = glm::angleAxis(rad.x, glm::vec3(1, 0, 0));
		glm::quat qy = glm::angleAxis(rad.y, glm::vec3(0, 1, 0));
		glm::quat qz = glm::angleAxis(rad.z, glm::vec3(0, 0, 1));
		// XYZ order:
		return glm::normalize(qz * qy * qx);
	}

	// Concatenate transforms: T * R * S
	inline glm::mat4 TRS(const glm::vec3& translation, const glm::quat& rotation, const glm::vec3& scale) {
		glm::mat4 T(1.0f);
		T = glm::translate(T, translation);
		T *= glm::mat4_cast(rotation);
		T = glm::scale(T, scale);
		return T;
	}

	inline std::pair<bool, float> Sample1D(const ofbx::AnimationCurve* curve, double t);

	inline TRSSample SampleTRS(const ofbx::Object* obj, const ofbx::AnimationLayer* layer, double t);

	static inline const ofbx::AnimationCurveNode* GetPropNode(const ofbx::Object* obj,const ofbx::AnimationLayer* layer, const char* prop);

	inline void BuildParentFirstSkeleton(const std::vector<const ofbx::Object*>& boneObjs, std::vector<int>& outOrder, const std::unordered_map<const ofbx::Object*, int>& indexOf);

	// To hold each animation 
	struct animImport {
		std::unique_ptr<Skeleton> skeleton;
		std::unique_ptr<AnimationPackage> animations;
	};

	class OFBXAnimationImporter {
	public:
		// Load and extract animation data from an FBX file
		static animImport importFromFile(const std::string& filename);
	};
}

#endif