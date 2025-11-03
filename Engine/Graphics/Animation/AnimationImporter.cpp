#include "pch.hpp"
#include "Animation.hpp"
#include "AnimationImporter.hpp"

#include "ofbx.h"

namespace EngineFramework
{
	std::vector<uint8_t> ReadFBXFile(const std::string& path) {
		// Buffer to hold file data
		std::vector<uint8_t> buffer;

		// Check if file is valid
		if (FILE* f = std::fopen(path.c_str(), "rb")) {
			// Find the end of file
			std::fseek(f, 0, SEEK_END);
			long sz = std::ftell(f);
			// Reset to the start
			std::fseek(f, 0, SEEK_SET);

			// Allocate space based on the size of the file obtained by sz
			buffer.resize((size_t)sz);
			// Read the file and close
			std::fread(buffer.data(), 1, buffer.size(), f);
			std::fclose(f);
		}

		return buffer;

	}

	TRSSample SampleTRS(const ofbx::Object* obj, const ofbx::AnimationLayer* layer, double t)
	{
		return TRSSample();
	}

	std::pair<bool, float> Sample1D(const ofbx::AnimationCurve* curve, double t) {
		// Verify if there is a curve pointer, this function is used to sample a value at t
		if (!curve) {
			return { false, 0.0 };
		}

		// Validate if there are more than 1 key
		int keyc = curve->getKeyCount();
		if (keyc == 0) {
			return { false, 0.0 };
		}
		if (keyc == 1) {
			return { true, curve->getKeyValue() };
		}

	}



	void BuildParentFirstSkeleton(const std::vector<const ofbx::Object*>& boneObjs, std::vector<int>& outOrder, const std::unordered_map<const ofbx::Object*, int>& indexOf) {

	}

	animImport OFBXAnimationImporter::importFromFile(const std::string& filename)
	{
		// 1) load FBX into ofbx::IScene*
		animImport bundle;
		auto data =
			// 2) build Skeleton (parent-first order, invBind)
			// 3) read animation stacks/layers; resample curves at targetFPS into BoneKeyframes
			// 4) package into AnimationPackage
			// 5) return bundle
	}
}