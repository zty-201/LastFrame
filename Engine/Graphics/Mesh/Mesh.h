/**
 * @file Mesh.h
 * @brief Provides utilities for generating and managing 3D mesh data.
 *
 * This file defines the `Mesh` namespace, which includes structures and functions
 * for creating, parsing, and manipulating 3D mesh data. It supports loading meshes
 * from files using Assimp, generating common 3D primitives (e.g., quads, cubes, spheres),
 * and calculating tangents for advanced rendering techniques.
 *
 * Key components:
 * - `Mesh::Vertex`: Represents a vertex with position, normal, UVs, and tangent.
 * - `Mesh::MeshData`: Stores vertex data, sub-mesh info, materials, and OpenGL handles.
 * - Utility functions for generating 3D primitives and calculating tangents.
 *
 * @author zty-201
 * @date 2025-11-03
 */
#ifndef MESH_H
#define MESH_H

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <limits>
#include <memory>
#include <cfloat>

#include "nlohmann/json.hpp"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "MaterialComponent.h"
#include "global.hpp" // For AssetManagerSystem, etc.
#include "AssimpWrapper.h"
#include "Animation.hpp"
#include <mutex>
#include "MeshComponent.h"
 //#include "AssetManager.hpp"

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

namespace Mesh {
	// Represents a single drawable part (mesh group) of a larger model.
	struct SubMesh {
		std::string name;
		int materialIndex = -1; // Index into the parent MeshData's materials vector.
		GLuint indexCount = 0;
		GLuint baseIndex = 0;
		GLuint baseVertex = 0;
	};

	// A flexible vertex struct for loading any kind of mesh.
	struct Vertex {
		glm::vec3 Position;
		glm::vec3 Normal;
		glm::vec2 TexCoords;
		glm::vec4 Tangent; // Using vec4 to include handedness (w component)

		// Add skinning data
		glm::ivec4 BoneIndices = glm::ivec4(0);
		glm::vec4 BoneWeights = glm::vec4(0.f);
	};

	struct AABB {
		glm::vec3 minV;
		glm::vec3 maxV;

		AABB()
			: minV(glm::vec3(FLT_MAX)),
			maxV(glm::vec3(-FLT_MAX)) {
		}

		glm::vec3 center() const { return 0.5f * (minV + maxV); }
		glm::vec3 halfExtents() const { return 0.5f * (maxV - minV); }

		void expand(const glm::vec3& p) {
			// safe now that macros are gone
			minV = glm::min(minV, p);
			maxV = glm::max(maxV, p);
		}
	};

	using IndexType = GLuint;
	using IndexBuffer = std::vector<IndexType>;

	// The main MeshData class, handling both loaded and generated meshes.
	class MeshData {
	public:
		// OpenGL handles for the entire model's geometry.
		GLuint vao = 0;
		GLuint vertexBuffer = 0;
		GLuint indexBuffer = 0;

		// A mesh is a collection of sub-meshes and their materials.
		std::vector<SubMesh> subMeshes;
		std::vector<MaterialComponent> materials;

		// Default constructor.
		MeshData() = default;

		// Constructor for loading a mesh from a file via Assimp. Call LoadWithFilename_AfterThreadJoinCall() after thread joins.
		explicit MeshData(const std::string& filePath, EngineFramework::Skeleton* skeleton = nullptr) {
			loadWithAssimp(filePath, skeleton);
		}

		const AABB& GetAABB() const { return m_aabb; }
		const std::vector<AABB>& GetSubmeshAABBs() const { return m_submeshAABBs; }

		// Constructor for manually creating a mesh from code (for primitives).
		explicit MeshData(const std::vector<Vertex>& vertices, const std::vector<GLuint>& indices) {
			SubMesh primitiveSubMesh;
			primitiveSubMesh.name = "default";
			primitiveSubMesh.materialIndex = 0;
			primitiveSubMesh.baseIndex = 0;
			primitiveSubMesh.baseVertex = 0;
			primitiveSubMesh.indexCount = static_cast<GLuint>(indices.size());
			subMeshes.push_back(primitiveSubMesh);

			MaterialComponent defaultMaterial;
			defaultMaterial.name = "default_material";
			materials.push_back(defaultMaterial);

			AABB subBox;
			for (const auto& v : vertices) {
				subBox.expand(v.Position);
				m_aabb.expand(v.Position);
			}
			m_submeshAABBs = { subBox };

			createGLBuffers(vertices, indices);
		}

		static void UpdateMeshBounds(entt::entity entity)
		{
			if (!registry.valid(entity) || !registry.all_of<MeshComponent>(entity)) return;

			auto& meshComp = registry.get<MeshComponent>(entity);

			// 1. Get the Resource
			using AM = EngineFramework::AssetManagerSystem;
			auto guid = AM::GetFullGuid(AM::MESH_GUID, meshComp.meshName);
			auto meshData = AM::GetResource<Mesh::MeshData>(guid);

			// 2. Sync Bounds from Resource to Component
			if (meshData) {
				const auto& aabb = meshData->GetAABB();
				meshComp.localMin[0] = aabb.minV.x;
				meshComp.localMin[1] = aabb.minV.y;
				meshComp.localMin[2] = aabb.minV.z;

				meshComp.localMax[0] = aabb.maxV.x;
				meshComp.localMax[1] = aabb.maxV.y;
				meshComp.localMax[2] = aabb.maxV.z;
			}
		}

		//To be called after MeshData load with filename completes, as it can only be completed on the main thread.
		void LoadWithFilename_AfterThreadJoinCall()
		{
			createGLBuffers(m_temp_vertices, m_temp_indices);
			m_temp_vertices.clear();
			m_temp_indices.clear();
		}
		std::string name;
		int num_vertices{};
		int num_bones{};
	private:
		AABB m_aabb;                     // whole-mesh AABB (object space)
		std::vector<AABB> m_submeshAABBs; // per-submesh AABBs

		// Assimp Convertion helper
		static inline glm::mat4 AssimpToGLM(const aiMatrix4x4& from) {
			glm::mat4 to;
			// Assimp is Row-Major, GLM is Column-Major. 
			// We usually need to transpose, but the memory layout of aiMatrix4x4 
			// often maps directly to glm::mat4 depending on how you access it.
			// The safest manual assignment:
			to[0][0] = from.a1; to[1][0] = from.a2; to[2][0] = from.a3; to[3][0] = from.a4;
			to[0][1] = from.b1; to[1][1] = from.b2; to[2][1] = from.b3; to[3][1] = from.b4;
			to[0][2] = from.c1; to[1][2] = from.c2; to[2][2] = from.c3; to[3][2] = from.c4;
			to[0][3] = from.d1; to[1][3] = from.d2; to[2][3] = from.d3; to[3][3] = from.d4;
			return to;
		}

		// The core Assimp loading function.
		void loadWithAssimp(const std::string& filePath, EngineFramework::Skeleton* skeleton = nullptr) {
			static std::mutex text_mutex;
			{
				std::lock_guard<std::mutex> text_lock(text_mutex);
				//LOGI("Loading mesh from file: %s", filePath.c_str());
			}
			Assimp::Importer importer;

			// Create custom IO system that can load files from Android assets
			auto ioSystem = new AssetIOSystem(filePath);
			importer.SetIOHandler(ioSystem);

			// Now we can use ReadFile() instead of ReadFileFromMemory()!
			// Assimp will use our custom IOSystem to load both .obj AND .mtl
			const aiScene* scene = importer.ReadFile(
				filePath,
				aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace
			);

			if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
				throw std::runtime_error("Assimp Error: " + std::string(importer.GetErrorString()));
			}

			for (unsigned int m = 0; m < scene->mNumMeshes; ++m) {
				const aiMesh* mesh = scene->mMeshes[m];
				this->num_bones += mesh->mNumBones;
				this->num_vertices += mesh->mNumVertices;
			}

			processMaterials(scene);
			processNode(scene->mRootNode, scene, skeleton);
		}



		// Extracts materials from the Assimp scene.
		void processMaterials(const aiScene* scene) {
			for (unsigned int i = 0; i < scene->mNumMaterials; i++) {
				aiMaterial* mat = scene->mMaterials[i];
				MaterialComponent newMat;
				newMat.name = mat->GetName().C_Str();

				using AssetMgr = EngineFramework::AssetManagerSystem;

				newMat.albedoFactor = glm::vec3(1.0f);
				newMat.roughnessFactor = 0.8f;
				newMat.metallicFactor = 0.0f;

				// Extract texture name from Assimp material
				auto tryLoadTexture = [&](aiTextureType type) -> std::string {
					aiString path;
					if (mat->GetTexture(type, 0, &path) == AI_SUCCESS && path.length > 0) {
						std::string pathStr(path.C_Str());

						// Replace backslashes with forward slashes
						std::replace(pathStr.begin(), pathStr.end(), '\\', '/');

						std::filesystem::path p(pathStr);
						std::string texName = p.stem().string();

						//LOGI("[%s] Found texture path: '%s' -> normalized: '%s' -> stem: '%s'",
						//    newMat.name.c_str(), path.C_Str(), pathStr.c_str(), texName.c_str());

						return texName;
					}
					return "";
					};

				// === ALBEDO ===
				std::string albedoName = tryLoadTexture(aiTextureType_DIFFUSE);
				if (!albedoName.empty()) {
					try {

						newMat.albedoMap = AssetMgr::GetFullGuid(AssetMgr::TEXTURE_GUID, albedoName);

						//LOGI(" Loaded albedo: %s", albedoName.c_str());
					}
					catch (const std::exception& e) {
						LOGE(" Albedo texture not found in AssetManager: %s (error: %s)",
						    albedoName.c_str(), e.what());
					}
				}

				// Fallback to color if no texture
				if (newMat.albedoMap.m_Instance.m_Value == 0) {
					aiColor3D color;
					if (mat->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS) {
						newMat.albedoFactor = { color.r, color.g, color.b };
						//LOGI("Using albedo color fallback: (%.2f, %.2f, %.2f)",
						//	color.r, color.g, color.b);
					}
				}

				// === NORMAL ===
				std::string normalName = tryLoadTexture(aiTextureType_NORMALS);
				if (normalName.empty()) {
					normalName = tryLoadTexture(aiTextureType_HEIGHT);
				}
				if (!normalName.empty()) {
					try {
						newMat.normalMap = AssetMgr::GetFullGuid(AssetMgr::TEXTURE_GUID, normalName);

						//LOGI(" Loaded normal: %s", normalName.c_str());
					}
					catch (...) {
						//LOGE(" Normal texture not found: %s", normalName.c_str());
					}
				}

				// === ROUGHNESS ===
				std::string roughnessName = tryLoadTexture(aiTextureType_DIFFUSE_ROUGHNESS);
				if (!roughnessName.empty()) {
					try {

						newMat.roughnessMap = AssetMgr::GetFullGuid(AssetMgr::TEXTURE_GUID, roughnessName);


						//LOGI(" Loaded roughness: %s", roughnessName.c_str());
					}
					catch (...) {}
				}
				else {
					float roughness;
					if (mat->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness) == AI_SUCCESS) {
						newMat.roughnessFactor = roughness;
					}
				}

				// === METALLIC  ===
				std::string metallicName = tryLoadTexture(aiTextureType_METALNESS);
				if (metallicName.empty()) {
					metallicName = tryLoadTexture(aiTextureType_SPECULAR);
				}
				if (!metallicName.empty()) {
					try {

						newMat.metallicMap = AssetMgr::GetFullGuid(AssetMgr::TEXTURE_GUID, metallicName);


						//LOGI(" Loaded metallic: %s", metallicName.c_str());
					}
					catch (...) {}
				}

				materials.push_back(newMat);
			}
		}

		// Recursively processes Assimp nodes.
		void processNode(aiNode* node, const aiScene* scene, EngineFramework::Skeleton* skeleton) {
			for (unsigned int i = 0; i < node->mNumMeshes; i++) {
				processSubMesh(scene->mMeshes[node->mMeshes[i]], scene, skeleton);
			}
			for (unsigned int i = 0; i < node->mNumChildren; i++) {
				processNode(node->mChildren[i], scene, skeleton);
			}
		}

		// Extracts vertex data from a single Assimp mesh.
		void processSubMesh(aiMesh* mesh, const aiScene* scene, EngineFramework::Skeleton* skeleton) {
			UNREFERENCED_PARAMETER(scene);
			SubMesh subMesh;
			subMesh.name = mesh->mName.C_Str();
			subMesh.materialIndex = mesh->mMaterialIndex;
			subMesh.baseVertex = static_cast<GLuint>(m_temp_vertices.size());
			subMesh.baseIndex = static_cast<GLuint>(m_temp_indices.size());
			subMesh.indexCount = 0;

			AABB subBox;

			for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
				Vertex vertex{};
				vertex.Position = { mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z };
				vertex.Normal = { mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z };
				if (mesh->mTextureCoords[0]) {
					vertex.TexCoords = { mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y };
				}
				if (mesh->mTangents) {
					// Assimp provides the tangent and bitangent, we use them to calculate the w component
					glm::vec3 t = { mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z };
					glm::vec3 b = { mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z };
					float handedness = (glm::dot(glm::cross(vertex.Normal, t), b) < 0.0f) ? -1.0f : 1.0f;
					vertex.Tangent = glm::vec4(t, handedness);
				}
				else {
					// === Generate tangent if Assimp didn't provide one ===
					glm::vec3 c1 = glm::cross(vertex.Normal, glm::vec3(0.0f, 0.0f, 1.0f));
					glm::vec3 c2 = glm::cross(vertex.Normal, glm::vec3(0.0f, 1.0f, 0.0f));
					glm::vec3 t = glm::length(c1) > glm::length(c2) ? c1 : c2;
					vertex.Tangent = glm::vec4(glm::normalize(t), 1.0f);
					//LOGE("Mesh '%s' missing tangents, using generated fallback", mesh->mName.C_Str());
				}

				// Add skinning data
				vertex.BoneIndices = glm::uvec4(0);
				vertex.BoneWeights = glm::vec4(0.0f);

				subBox.expand(vertex.Position);
				m_aabb.expand(vertex.Position);

				m_temp_vertices.push_back(vertex);
			}

			for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
				aiFace face = mesh->mFaces[i];
				for (unsigned int j = 0; j < face.mNumIndices; j++) {
					m_temp_indices.push_back(subMesh.baseVertex + face.mIndices[j]);
				}
				subMesh.indexCount += face.mNumIndices;
			}

			// Fill bone weights and indices
			if (mesh->HasBones()) {
				const unsigned int numBones = mesh->mNumBones;

				for (unsigned int b = 0; b < numBones; ++b) {
					const aiBone* bone = mesh->mBones[b];

					// Start with local index, then try to remap via OFBX skeleton
					unsigned int boneIndex = b;
					{
						std::lock_guard<std::mutex> bone_lock{ EngineFramework::g_OFBXBoneIndexByName_mutex };
						auto itRemap = EngineFramework::g_OFBXBoneIndexByName.find(bone->mName.C_Str());
						if (itRemap != EngineFramework::g_OFBXBoneIndexByName.end()) {
							boneIndex = static_cast<unsigned int>(itRemap->second);
						}
						else {
							LOGE("[SKIN] Assimp bone '%s' not found in OFBX skeleton; using local index %u",
								bone->mName.C_Str(), boneIndex);
						}
					}


					//LOGI("[SKIN] Assimp bone %u ('%s') -> OFBX index %u",
					//	b, bone->mName.C_Str(), boneIndex);

					if (skeleton != nullptr && boneIndex < skeleton->bones.size()) {
						skeleton->bones[boneIndex].invBind = AssimpToGLM(bone->mOffsetMatrix);
					}

					//if (skeleton && boneIndex < skeleton->bones.size()) {
					//    // Overwrite the manual OpenFBX matrix with Assimp's correct matrix
					//    skeleton->bones[boneIndex].invBind = AssimpToGLM(bone->mOffsetMatrix);

					//    // Optional: Debugging
					//    // LOGI("Corrected InvBind for bone %s", bone->mName.C_Str());
					//}

					// Loop through weights for this bone
					for (unsigned int w = 0; w < bone->mNumWeights; ++w) {
						const aiVertexWeight& vw = bone->mWeights[w];
						unsigned int vtxId = vw.mVertexId;   // local to this mesh
						float weight = vw.mWeight;

						// Get the vertex in global array for this submesh
						Vertex& v = m_temp_vertices[subMesh.baseVertex + vtxId];

						// Put weight into the first empty slot
						for (int k = 0; k < 4; ++k) {
							if (v.BoneWeights[k] == 0.0f) {
								v.BoneWeights[k] = weight;
								v.BoneIndices[k] = static_cast<int>(boneIndex);
								break;
							}
						}
					}
				}

				// Normalize weights so each vertex sums to 1 where possible
				for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
					Vertex& v = m_temp_vertices[subMesh.baseVertex + i];
					float sum =
						v.BoneWeights.x + v.BoneWeights.y +
						v.BoneWeights.z + v.BoneWeights.w;

					if (sum > 0.0f) {
						v.BoneWeights /= sum;
					}
				}
			}

			subMeshes.push_back(subMesh);
			m_submeshAABBs.push_back(subBox);
		}

		// Creates the final OpenGL VAO and buffers.
		void createGLBuffers(const std::vector<Vertex>& vertices, const std::vector<GLuint>& indices) {
			glGenVertexArrays(1, &vao);
			glBindVertexArray(vao);

			glGenBuffers(1, &vertexBuffer);
			glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
			glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

			glGenBuffers(1, &indexBuffer);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);

			glEnableVertexAttribArray(0); // Position (location 0)
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Position));

			glEnableVertexAttribArray(1); // TexCoords (location 1)
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));

			glEnableVertexAttribArray(2); // Normal (location 2)
			glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));

			glEnableVertexAttribArray(3); // Tangent (location 3)
			glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Tangent));

			glEnableVertexAttribArray(4); // Bone Indices (location 5)
			glVertexAttribIPointer(4, 4, GL_INT, sizeof(Vertex), (void*)offsetof(Vertex, BoneIndices));

			glEnableVertexAttribArray(5); // Bone Weights (location 5)
			glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, BoneWeights));

			glBindVertexArray(0);
		}

	private:
		std::vector<Vertex> m_temp_vertices;
		std::vector<GLuint> m_temp_indices;
	};

	// =========================================================================
	// Functions to generate primitive meshes from code    
	// =========================================================================

	// Generic helper function to calculate tangents for any manually created mesh
	inline void calculateTangents(std::vector<Vertex>& vertices, const std::vector<GLuint>& indices) {
		if (indices.empty() || vertices.empty()) return;

		std::vector<glm::vec3> tan1(vertices.size(), glm::vec3(0.0f));
		std::vector<glm::vec3> tan2(vertices.size(), glm::vec3(0.0f));

		for (size_t i = 0; i < indices.size(); i += 3) {
			Vertex& v0 = vertices[indices[i]];
			Vertex& v1 = vertices[indices[i + 1]];
			Vertex& v2 = vertices[indices[i + 2]];
			glm::vec3 edge1 = v1.Position - v0.Position;
			glm::vec3 edge2 = v2.Position - v0.Position;
			glm::vec2 deltaUV1 = v1.TexCoords - v0.TexCoords;
			glm::vec2 deltaUV2 = v2.TexCoords - v0.TexCoords;
			float det = (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);
			if (std::abs(det) < 1e-6f) continue;
			float f = 1.0f / det;
			glm::vec3 tangent, bitangent;
			tangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
			tangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
			tangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
			bitangent.x = f * (-deltaUV1.x * edge2.x + deltaUV2.x * edge1.x);
			bitangent.y = f * (-deltaUV1.x * edge2.y + deltaUV2.x * edge1.y);
			bitangent.z = f * (-deltaUV1.x * edge2.z + deltaUV2.x * edge1.z);
			tan1[indices[i]] += tangent;
			tan1[indices[i + 1]] += tangent;
			tan1[indices[i + 2]] += tangent;
			tan2[indices[i]] += bitangent;
			tan2[indices[i + 1]] += bitangent;
			tan2[indices[i + 2]] += bitangent;
		}

		for (size_t i = 0; i < vertices.size(); ++i) {
			const glm::vec3& n = vertices[i].Normal;
			const glm::vec3& t = tan1[i];
			if (glm::length(t) < 1e-6f) {
				vertices[i].Tangent = glm::vec4(1, 0, 0, 1); // Default tangent
				continue;
			}
			glm::vec3 tangent = glm::normalize(t - n * glm::dot(n, t));
			float handedness = (glm::dot(glm::cross(n, t), tan2[i]) < 0.0f) ? -1.0f : 1.0f;
			vertices[i].Tangent = glm::vec4(tangent, handedness);
		}
	}

	// Generates a standard cube
	inline std::shared_ptr<MeshData> generateCube() {
		std::vector<Vertex> vertices = {
			// Positions, Normals, UVs
			{{-0.5f, -0.5f, -0.5f}, {0.0f,  0.0f, -1.0f}, {0.0f, 0.0f}}, {{0.5f,  0.5f, -0.5f}, {0.0f,  0.0f, -1.0f}, {1.0f, 1.0f}}, {{0.5f, -0.5f, -0.5f}, {0.0f,  0.0f, -1.0f}, {1.0f, 0.0f}}, {{-0.5f,  0.5f, -0.5f}, {0.0f,  0.0f, -1.0f}, {0.0f, 1.0f}},
			{{-0.5f, -0.5f,  0.5f}, {0.0f,  0.0f,  1.0f}, {0.0f, 0.0f}}, {{0.5f, -0.5f,  0.5f}, {0.0f,  0.0f,  1.0f}, {1.0f, 0.0f}}, {{0.5f,  0.5f,  0.5f}, {0.0f,  0.0f,  1.0f}, {1.0f, 1.0f}}, {{-0.5f,  0.5f,  0.5f}, {0.0f,  0.0f,  1.0f}, {0.0f, 1.0f}},
			{{-0.5f,  0.5f,  0.5f}, {-1.0f,  0.0f,  0.0f}, {1.0f, 0.0f}}, {{-0.5f,  0.5f, -0.5f}, {-1.0f,  0.0f,  0.0f}, {1.0f, 1.0f}}, {{-0.5f, -0.5f, -0.5f}, {-1.0f,  0.0f,  0.0f}, {0.0f, 1.0f}}, {{-0.5f, -0.5f,  0.5f}, {-1.0f,  0.0f,  0.0f}, {0.0f, 0.0f}},
			{{0.5f,  0.5f,  0.5f}, {1.0f,  0.0f,  0.0f}, {1.0f, 0.0f}}, {{0.5f, -0.5f, -0.5f}, {1.0f,  0.0f,  0.0f}, {0.0f, 1.0f}}, {{0.5f,  0.5f, -0.5f}, {1.0f,  0.0f,  0.0f}, {1.0f, 1.0f}}, {{0.5f, -0.5f,  0.5f}, {1.0f,  0.0f,  0.0f}, {0.0f, 0.0f}},
			{{-0.5f, -0.5f, -0.5f}, {0.0f, -1.0f,  0.0f}, {0.0f, 1.0f}}, {{0.5f, -0.5f, -0.5f}, {0.0f, -1.0f,  0.0f}, {1.0f, 1.0f}}, {{0.5f, -0.5f,  0.5f}, {0.0f, -1.0f,  0.0f}, {1.0f, 0.0f}}, {{-0.5f, -0.5f,  0.5f}, {0.0f, -1.0f,  0.0f}, {0.0f, 0.0f}},
			{{-0.5f,  0.5f, -0.5f}, {0.0f,  1.0f,  0.0f}, {0.0f, 1.0f}}, {{0.5f,  0.5f,  0.5f}, {0.0f,  1.0f,  0.0f}, {1.0f, 0.0f}}, {{0.5f,  0.5f, -0.5f}, {0.0f,  1.0f,  0.0f}, {1.0f, 1.0f}}, {{-0.5f,  0.5f,  0.5f}, {0.0f,  1.0f,  0.0f}, {0.0f, 0.0f}}
		};
		std::vector<GLuint> indices = { 0, 1, 2, 0, 3, 1, 4, 5, 6, 4, 6, 7, 8, 9, 10, 8, 10, 11, 12, 13, 14, 12, 15, 13, 16, 17, 18, 16, 18, 19, 20, 21, 22, 20, 23, 21 };
		calculateTangents(vertices, indices);
		return std::make_shared<MeshData>(vertices, indices);
	}

	// Generates a UV sphere
	inline std::shared_ptr<MeshData> generateSphere(int xSegments = 64, int ySegments = 64) {
		std::vector<Vertex> vertices;
		std::vector<GLuint> indices;
		for (int y = 0; y <= ySegments; ++y) {
			for (int x = 0; x <= xSegments; ++x) {
				float xSegment = (float)x / (float)xSegments;
				float ySegment = (float)y / (float)ySegments;
				float xPos = std::cos(xSegment * 2.0f * PI) * std::sin(ySegment * PI);
				float yPos = std::cos(ySegment * PI);
				float zPos = std::sin(xSegment * 2.0f * PI) * std::sin(ySegment * PI);
				vertices.push_back({ {xPos, yPos, zPos}, {xPos, yPos, zPos}, {xSegment, ySegment} });
			}
		}
		for (int y = 0; y < ySegments; ++y) {
			for (int x = 0; x < xSegments; ++x) {
				indices.push_back((y + 1) * (xSegments + 1) + x);
				indices.push_back(y * (xSegments + 1) + x);
				indices.push_back(y * (xSegments + 1) + x + 1);
				indices.push_back((y + 1) * (xSegments + 1) + x);
				indices.push_back(y * (xSegments + 1) + x + 1);
				indices.push_back((y + 1) * (xSegments + 1) + x + 1);
			}
		}
		calculateTangents(vertices, indices);
		return std::make_shared<MeshData>(vertices, indices);
	}

	// Generates a simple quad
	inline std::shared_ptr<MeshData> generateQuad() {
		std::vector<Vertex> vertices = {
			{{-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
			{{ 1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
			{{ 1.0f,  1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
			{{-1.0f,  1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}}
		};
		std::vector<GLuint> indices = { 0, 1, 2, 2, 3, 0 };
		calculateTangents(vertices, indices);
		return std::make_shared<MeshData>(vertices, indices);
	}

	// Generates a cube for skybox rendering (no normals, UVs, or tangents needed)
	inline std::shared_ptr<MeshData> generateSkyBox() {
		std::vector<Vertex> vertices = {
			{{-1.0f,  1.0f, -1.0f}}, {{-1.0f, -1.0f, -1.0f}}, {{1.0f, -1.0f, -1.0f}}, {{1.0f,  1.0f, -1.0f}},
			{{-1.0f, -1.0f,  1.0f}}, {{-1.0f,  1.0f,  1.0f}}, {{1.0f, -1.0f,  1.0f}}, {{1.0f,  1.0f,  1.0f}}
		};
		std::vector<GLuint> indices = { 0, 1, 2, 0, 2, 3, 1, 4, 6, 1, 6, 2, 4, 5, 7, 4, 7, 6, 5, 0, 3, 5, 3, 7, 3, 2, 6, 3, 6, 7, 0, 5, 4, 0, 4, 1 };
		return std::make_shared<MeshData>(vertices, indices);
	}

} // namespace Mesh

#endif // MESH_H