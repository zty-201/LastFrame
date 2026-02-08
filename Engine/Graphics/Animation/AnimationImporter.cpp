#include <stdio.h>
#include "pch.hpp"
#include "Animation.hpp"
#include "AnimationImporter.hpp"
namespace EngineFramework
{
	std::unordered_map<std::string, int> g_OFBXBoneIndexByName;
	std::mutex g_OFBXBoneIndexByName_mutex;

	// Animation flags
	ofbx::u64 animFlags =
		(ofbx::u64)ofbx::LoadFlags::IGNORE_BLEND_SHAPES |
		(ofbx::u64)ofbx::LoadFlags::IGNORE_CAMERAS |
		(ofbx::u64)ofbx::LoadFlags::IGNORE_LIGHTS |
		(ofbx::u64)ofbx::LoadFlags::IGNORE_TEXTURES |
		(ofbx::u64)ofbx::LoadFlags::IGNORE_MATERIALS |
		(ofbx::u64)ofbx::LoadFlags::IGNORE_VIDEOS |
		(ofbx::u64)ofbx::LoadFlags::IGNORE_POSES;

	//std::vector<uint8_t> ReadFBXFile(const std::string& path) {
	//	// Buffer to hold file data
	//	std::vector<uint8_t> buffer;

	//	// Check if file is valid
	//	FILE* f = nullptr;
	//	fopen_s(&f, path.c_str(), "rb");
	//	if (f) {
	//		// Find the end of file
	//		std::fseek(f, 0, SEEK_END);
	//		long sz = std::ftell(f);
	//		// Reset to the start
	//		std::fseek(f, 0, SEEK_SET);

	//		// Allocate space based on the size of the file obtained by sz
	//		buffer.resize((size_t)sz);
	//		// Read the file and close
	//		std::fread(buffer.data(), 1, buffer.size(), f);
	//		std::fclose(f);
	//	}

	//	return buffer;

	//}

	std::pair<bool, float> Sample1D(const ofbx::AnimationCurve* curve, double t) {
		// Verify if there is a curve pointer, this function is used to sample a value at t
		if (!curve) {
			return { false, 0.0f };
		}

		// Validate if there are more than 1 key
		int keyc = curve->getKeyCount();
		if (keyc == 0) {
			return { false, 0.0f };
		}
		if (keyc == 1) {
			return { true, curve->getKeyValue()[0] };
		}

		// Binary search for the right keyframe
		int low = 0, high = keyc - 1;
		// Obtain the times and values
		const auto* times = curve->getKeyTime();
		const auto* values = curve->getKeyValue();
		const ofbx::i64 T = (ofbx::i64)llround(t * 46186158000.0); // FBX time unit is 1/46186158000 seconds

		while (low < high) {
			int mid = (low + high + 1) >> 1;
			if (static_cast<double>(times[mid]) <= T) {
				low = mid;
			}
			else {
				high = mid - 1;
			}
		}

		// low is now the index of the keyframe at or before time t
		const int i0 = low;
		const int i1 = std::min(i0 + 1, keyc - 1);

		// Obtain the times of the frames	
		const double t0 = static_cast<double>(times[i0]);
		const double t1 = static_cast<double>(times[i1]);

		// Obtain the values of the frames
		const float v0 = values[i0];
		const float v1 = values[i1];

		double u = (t1 > t0) ? (T - t0) / (t1 - t0) : 0.0;
		u = std::clamp(u, 0.0, 1.0);

		return { true, static_cast<float>(v0 * (1.0 - u) + v1 * u) };
	}

	TRSSample SampleTRS(const ofbx::Object* obj, const ofbx::AnimationLayer* layer, double t)
	{
		TRSSample out;

		// Translation
		if (const auto* tnode = GetPropNode(obj, layer, "Lcl Translation")) {
			const auto sx = Sample1D(tnode->getCurve(0), t);
			const auto sy = Sample1D(tnode->getCurve(1), t);
			const auto sz = Sample1D(tnode->getCurve(2), t);
			if (sx.first && sy.first && sz.first) {
				out.T = glm::vec3(sx.second, sy.second, sz.second);
				out.hasT = true;
			}
		}

		// Rotation (Euler degrees)
		if (const auto* rnode = GetPropNode(obj, layer, "Lcl Rotation")) {
			const auto sx = Sample1D(rnode->getCurve(0), t);
			const auto sy = Sample1D(rnode->getCurve(1), t);
			const auto sz = Sample1D(rnode->getCurve(2), t);
			if (sx.first && sy.first && sz.first) {
				out.Rdeg = glm::vec3(sx.second, sy.second, sz.second);
				out.hasR = true;
			}
		}

		// Scaling
		if (const auto* snode = GetPropNode(obj, layer, "Lcl Scaling")) {
			const auto sx = Sample1D(snode->getCurve(0), t);
			const auto sy = Sample1D(snode->getCurve(1), t);
			const auto sz = Sample1D(snode->getCurve(2), t);
			if (sx.first && sy.first && sz.first) {
				out.S = glm::vec3(sx.second, sy.second, sz.second);
				out.hasS = true;
			}
		}

		// Pre & Post Rotations
		ofbx::DVec3 pr = obj->getPreRotation();
		ofbx::DVec3 por = obj->getPostRotation();

		out.preR = glm::vec3(pr.x, pr.y, pr.z);
		out.postR = glm::vec3(por.x, por.y, por.z);

		// Rotation order
		out.rotOrder = obj->getRotationOrder();

		return out;
	}

	// Get the animation curve node
	const ofbx::AnimationCurveNode* GetPropNode(const ofbx::Object* obj, const ofbx::AnimationLayer* layer, const char* prop)
	{
		// Verify if the obj is valid
		if (!obj) {
			return nullptr;
		}
		// Check if layer exists before getting the Curve Node
		if (layer) {
			if (auto* n = layer->getCurveNode(*obj, prop)) return n;
		}

		return static_cast<const ofbx::AnimationCurveNode*>(obj->resolveObjectLink(ofbx::Object::Type::ANIMATION_CURVE_NODE, prop, 0));
	}

	void BuildParentFirstSkeleton(const std::vector<const ofbx::Object*>& boneObjs, std::vector<int>& outOrder, const std::unordered_map<const ofbx::Object*, int>& indexOf) {
		const int n = (int)boneObjs.size();
		std::vector<uint8_t> vis(n, 0);

		std::function<void(int)> dfs = [&](int i) {
			vis[i] = 1;
			for (int j = 0; j < n; ++j) {
				if (vis[j]) continue;
				if (boneObjs[j]->getParent() == boneObjs[i]) dfs(j);
			}
			outOrder.push_back(i); // postorder
			};

		for (int i = 0; i < n; ++i) {
			if (!boneObjs[i]->getParent() || !indexOf.count(boneObjs[i]->getParent())) {
				dfs(i);
			}
		}
		std::reverse(outOrder.begin(), outOrder.end());
	}

	animImport OFBXAnimationImporter::importFromFile(const std::string& filename)
	{
		// 1) load FBX into ofbx::IScene*
		animImport bundle;
		auto data = AssetManagerSystem::GetDataFromFile(filename);

		// Check for successful read
		if (data.empty()) {
			throw std::runtime_error("Failed to read FBX file: " + filename);
		}

		// Obtain the scene
		int byteCount = static_cast<int>(data.size());
		std::unique_ptr<ofbx::IScene, SceneDeleter> scene(
			ofbx::load(data.data(), byteCount, static_cast<ofbx::u16>(animFlags))
		);

		// Check if scene was properly parsed
		if (!scene) {
			throw std::runtime_error("Failed to load FBX scene: " + filename);
		}

		// 2) build Skeleton (parent-first order, invBind)
		// Collect the bones
		std::vector<const ofbx::Object*> boneObjs;
		boneObjs.reserve(scene->getAllObjectCount());

		// Get each bone / limb node
		for (int i = 0; i < scene->getAllObjectCount(); i++) {
			// Get the object
			if (const ofbx::Object* obj = scene->getAllObjects()[i]) {
				// Check if object is a limb node
				if (obj->getType() == ofbx::Object::Type::LIMB_NODE) {
					boneObjs.push_back(obj);
				}
			}
		}


		// Else if no bones found, return empty bundle
		if (boneObjs.empty()) {
			bundle.skeleton = std::make_unique<Skeleton>();
			bundle.animations = std::make_unique<AnimationPackage>();
			return bundle;
		}

		// Map for quick index lookup
		std::unordered_map<const ofbx::Object*, int> indexOf;
		indexOf.reserve(boneObjs.size());
		for (int i = 0; i < (int)boneObjs.size(); i++) {
			indexOf[boneObjs[i]] = i;
		}

		// 3) read animation stacks/layers; resample curves at targetFPS into BoneKeyframes
		std::vector<int> order;
		BuildParentFirstSkeleton(boneObjs, order, indexOf);

		// 4) package into AnimationPackage
		auto skeleton = std::make_unique<Skeleton>();
		skeleton->bones.resize(boneObjs.size());

		// Obtain global for bones that are linked in a cluster
		// Loop through meshes to find clusters
		std::unordered_map<int, glm::mat4> boneGlobalMap;
		std::unordered_map<int, glm::mat4> boneInvBindMap;
		for (int i = 0; i < scene->getMeshCount(); i++) {
			// Check for valid mesh
			const ofbx::Mesh* mesh = scene->getMesh(i);
			if (!mesh) { continue; }

			// Check for valid geometry
			const ofbx::Geometry* geo = mesh->getGeometry();
			if (!geo) { continue; }

			// Check for valid skin
			const ofbx::Skin* skin = geo->getSkin();
			if (!skin) { continue; }

			for (int j = 0; j < skin->getClusterCount(); j++) {
				// Check for valid cluster
				const ofbx::Cluster* cluster = skin->getCluster(j);
				if (!cluster) { continue; }

				// Check for valid link for bone objects
				const ofbx::Object* link = cluster->getLink();
				if (!link) { continue; }

				// Find if it is part of the map indexOf
				auto iter = indexOf.find(link);
				if (iter == indexOf.end()) { continue; }

				int index = iter->second;

				// FBX gives: TransformLinkMatrix (bone global bind), TransformMatrix (geometry global bind)
				// We need bone global bind to compute invBind = inverse(boneGlobalBind).
				ofbx::DMatrix linkBind = cluster->getTransformLinkMatrix();
				glm::mat4 B = mat4Converter(linkBind);
				boneGlobalMap[index] = B;

				// Geometry global at bind (M) same for all clusters of this skin
				ofbx::DMatrix geomBindDM = cluster->getTransformMatrix();
				glm::mat4 M = mat4Converter(geomBindDM);

				// Precompute the full inverse-bind
				glm::mat4 invBind = glm::inverse(B);
				boneInvBindMap[index] = invBind;

			}
		}

		// Fill skeleton->bones in the SAME order as boneObjs
		// so bone index i == cluster index i == vertex bone index.
		for (int i = 0; i < (int)boneObjs.size(); ++i)
		{
			const ofbx::Object* obj = boneObjs[i];

			Bone bone{};
			bone.name = obj->name;

			// Parent index in boneObjs space
			int parent = -1;
			const ofbx::Object* parentObj = obj->getParent();

			// TRAVERSE UP until we find a parent that exists in our loaded bone list
			// or we hit the root (nullptr)
			while (parentObj) {
				auto it = indexOf.find(parentObj);
				if (it != indexOf.end()) {
					parent = it->second;
					break; // Found a valid bone parent!
				}
				// This node wasn't a bone (maybe a helper/group), keep going up
				parentObj = parentObj->getParent();
			}

			bone.parentIndex = parent;

			// ---------------- Global bind pose (B) ----------------
			glm::mat4 globalBind;
			auto itB = boneGlobalMap.find(i);
			if (itB != boneGlobalMap.end()) {
				globalBind = itB->second;
			}
			else {
				globalBind = mat4Converter(obj->getGlobalTransform());
			}

			// The ROOT ROTATION FIX (Z-up to Y-up) MUST STAY:
				// This fixes the 'lying down' issue.
			//if (bone.parentIndex == -1) {
			//	glm::mat4 fixRot = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1, 0, 0));
			//	globalBind = fixRot * globalBind;
			//}

			

			// The simplified invBind calculation must stay (assuming M is baked into vertices):
			auto itBI = boneInvBindMap.find(i);
			if (itBI != boneInvBindMap.end()) {
				bone.invBind = itBI->second;
			} else {
				bone.invBind = glm::inverse(globalBind);
			}

			// ---------------- Optional local bind (neutralPose) ----------------
			if (parent >= 0)
			{
				const Bone& parentBone = skeleton->bones[parent];
				glm::mat4 parentGlobal = glm::inverse(parentBone.invBind);
				bone.neutralPose = glm::inverse(parentGlobal) * globalBind;
			}
			else
			{
				bone.neutralPose = globalBind;
			}

			skeleton->bones[i] = bone;
		}

		// After filling skeleton->bones
		{
			std::lock_guard<std::mutex> bone_mutex(g_OFBXBoneIndexByName_mutex);
			g_OFBXBoneIndexByName.clear();
			for (int i = 0; i < (int)boneObjs.size(); ++i)
			{
				const char* n = boneObjs[i]->name;
				g_OFBXBoneIndexByName[std::string(n)] = i;
			}
		}
		

		// 5) return bundle (Animation Package)
		auto package = std::make_unique<AnimationPackage>();

		int count = scene->getAnimationStackCount();

		for (int i = 0; i < count; i++) {
			const ofbx::AnimationStack* stack = scene->getAnimationStack(i);
			if (!stack) continue;

			const ofbx::AnimationLayer* layer = stack->getLayer(0);
			if (!layer) continue;

			Animation anim{};
			anim.fps = 30;
			anim.name = stack->name;

			double tMin = 1e30;
			double tMax = -1e30;
			bool   anyKeys = false;

			// Allocate memory based on the keyframes
			anim.boneKeyframes.resize(boneObjs.size());

			for (int j = 0; j < boneObjs.size(); j++) {
				const ofbx::Object* obj = boneObjs[j];
				const ofbx::AnimationCurveNode* tNode = GetPropNode(obj, layer, "Lcl Translation");
				const ofbx::AnimationCurveNode* rNode = GetPropNode(obj, layer, "Lcl Rotation");
				const ofbx::AnimationCurveNode* sNode = GetPropNode(obj, layer, "Lcl Scaling");

				// Helper function to obtain clip time range
				auto timeRange = [&](const ofbx::AnimationCurveNode* n) {
					if (!n) { return; }
					for (int k = 0; k < 3; k++) {
						// Check if curve is valid
						const ofbx::AnimationCurve* curve = n->getCurve(k);
						if (!curve) { continue; }

						// Check if key count is greater than 0
						int keyCount = curve->getKeyCount();
						if (keyCount == 0) { continue; }

						// Find the start time and end time
						anyKeys = true;
						const ofbx::i64* t0 = curve->getKeyTime();

						// Check for highest and lowest
						for (int l = 0; l < keyCount; l++) {
							double ts = static_cast<double>(t0[l]) / 46186158000LL;
							tMin = std::min(tMin, ts);
							tMax = std::max(tMax, ts);
						}
					}
					};

				timeRange(tNode);
				timeRange(rNode);
				timeRange(sNode);
			}

			// Check if valid keyframes
			if (!anyKeys) {
				// If no key, create a 1 frame idle so that no crash at runtime
				anim.duration = 0.f;
				anim.boneKeyframes.resize(boneObjs.size());
				for (auto& track : anim.boneKeyframes) {
					track.frames.resize(1);
					track.frames[0] = keyFrame{};
				}
				package->animations.push_back(std::move(anim));
				continue;
			}

			// Set duration of animation clip
			if (!(tMax > tMin)) {
				tMax = tMin + 1.0 / anim.fps;
			}

			anim.duration = static_cast<float>(tMax - tMin);
			const double dt = 1.0 / anim.fps;
			const int    numFrames = std::max(1, int(std::ceil((tMax - tMin) * anim.fps)));

			// --------------------------------------------
			// 1) First build tracks in boneObjs-index space
			// --------------------------------------------
			// Resample each bone at the fixed FPS
			anim.boneKeyframes.clear();
			anim.boneKeyframes.resize(boneObjs.size());

			for (int j = 0; j < (int)boneObjs.size(); j++)
			{
				const ofbx::Object* obj = boneObjs[j];
				auto& animTrack = anim.boneKeyframes[j];
				animTrack.frames.resize(numFrames);

				for (int k = 0; k < numFrames; k++)
				{
					double t = tMin + k * dt;

					TRSSample s = SampleTRS(obj, layer, t);
					keyFrame tr;
					if (s.hasT) {
						tr.position = s.T;
					}
					else {
						// Fallback to the static bind pose position
						ofbx::DVec3 v = obj->getLocalTranslation();
						tr.position = glm::vec3(v.x, v.y, v.z);
					}

					if (s.hasR) {
						tr.rotation = ConvertEuler(s.Rdeg, s.rotOrder);
					}
					else {
						// Fallback to the static bind pose rotation
						ofbx::DVec3 v = obj->getLocalRotation();
						// Convert Euler to Quat (assuming same rotation order)
						tr.rotation = ConvertEuler(glm::vec3(v.x, v.y, v.z), s.rotOrder);
					}

					if (s.hasS) {
						tr.scale = s.S;
					}
					else {
						// Fallback to the static bind pose scale
						ofbx::DVec3 v = obj->getLocalScaling();
						tr.scale = glm::vec3(v.x, v.y, v.z);
					}

					if (skeleton->bones[j].parentIndex == -1) {
						// Apply the same -90 X rotation to the animation keyframe
						glm::quat fixQ = glm::angleAxis(glm::radians(-90.0f), glm::vec3(1, 0, 0));
						tr.rotation = fixQ * tr.rotation;
					}

					
					// NEW: copy FBX bone constants onto every keyframe
					tr.preRotDeg = s.preR;
					tr.postRotDeg = s.postR;
					tr.rotOrder = s.rotOrder;

					animTrack.frames[k] = tr;
				}
			}

			//// --------------------------------------------
			//// 2) Now reorder tracks into parent-first order
			////     so index i matches skeleton->bones[i]
			//// --------------------------------------------
			//anim.boneKeyframes.clear();
			//anim.boneKeyframes.resize(order.size());

			//for (int iBone = 0; iBone < (int)order.size(); ++iBone) {
			//	int objIndex = order[iBone];           // index in boneObjs
			//	anim.boneKeyframes[iBone] = std::move(rawTracks[objIndex]);
			//}

			package->animations.push_back(std::move(anim));
		}

		// Done and return
		bundle.skeleton = std::move(skeleton);
		bundle.animations = std::move(package);
		return bundle;
	}
}