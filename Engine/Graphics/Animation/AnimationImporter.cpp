#include "pch.hpp"
#include "Animation.hpp"
#include "AnimationImporter.hpp"

#include "ofbx.h"

namespace EngineFramework
{
	std::vector<uint8_t> ReadFBXFile(const std::string& path) {
		std::vector<uint8_t> buffer;

		if(FILE* f = std::fopen(path.c_str(), "rb")) {
			std::fseek(f, 0, SEEK_END);
			long sz = std::ftell(f);
			std::fseek(f, 0, SEEK_SET);
			buffer.resize((size_t)sz);
			std::fread(buffer.data(), 1, buffer.size(), f);
			std::fclose(f);
		}

		return buffer;

	}

	std::pair<bool, double> Sample1D(const ofbx::AnimationCurve* curve, double t) {
		return std::pair<bool, double>();
	}

	TRSSample SampleTRS(const ofbx::Object* obj, const ofbx::AnimationLayer* layer, double t) {
		return TRSSample();
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