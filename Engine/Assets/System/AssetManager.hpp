/**
 * @file    AssetManager.hpp
 * @author  Mohamed Arshad Bin Mohamed Hussain (mohamedarshad.b), Joel Lee Jie (joeljie.lee)
 * @brief   As of 14 September 2025, Path-only Asset Manager (no file parsing).
 *
 * This manager keeps a database of asset name to relative path under a
 * chosen root folder. It does NOT open files (probably including stb_image in the future).
 * It helps you:
 *  - register assets,
 *  - check if files exist on disk,
 *  - get relative/absolute paths,
 *  - move/copy files inside the root (or only update the record),
 *  - rename asset,
 *  - import a whole folder by extension,
 *  - list all assets.
 *
*/

/*
		Asset Manager.
		Resources are loaded at initialise time, and are assigned a name->guid mapping in the guid.meta file located within the asset folder, for the first ever load.
		This guid will and should stay constant no matter what afterwards.

		To load an asset, a loader function bool(const std::filesystem::path&) is added to a <name, func> asset_loader_map in Initialize().
		This name must match the name given in ASSET_CONFIG_FILE defined in global.hpp.

		During Initialization, ASSET_CONFIG_FILE is read which gives <name, folderpath> json pairs. For each pair, the respective function loader is called
		and passed a folderpath.

		This loader function takes in a folderpath, and loads all assets of <T> inside. It may choose to use a separate config file within the folder, or load all assets as is.
		It generates a type_guid for all assets of <T> as well gets the corresponding instance_guid from guid.meta file mapping based on the asset name (not asset filepath)
		into a full_guid.
		The loaded assets of type std::shared_ptr<T> is then added to the resource_map via AddResource, together with its full_guid.
		Upon any failure, it is to return false. Otherwise if all assets are loaded, return true.

		instance_guid --> generated randomly upon first load of an asset, and kept constant. name->guid mapping is written in guid.meta file.
		type_guid --> dependent on asset type name
		full_guid --> consists of instance and type guid.

		Usage==
		0. Replacing asset (same name)
		- Simply reload the asset of X type, then all instances will simply used that new asset.

		0. Renaming Asset
		- Note: Asset path should stay in the same folder.
		- After renaming the asset file itself, parse guid.meta file and look for the old asset name.
		- change that old name to the new name. e.g. "old_name" -> guid becomes "new_name" -> guid.
		- Finally, change guid_to_name mapping (for level editor display of guids as readable names). 
		- Unless accounted for in code, name_to_guid mapping should not change. otherwise for example a graphic system using "Basic" shader will not be able to find it any longer.

		1. Accessing Asset
		- For every component, all asset types like texture, shader, mesh, audio file etc. are stored as xresource::full_guid.
		- Use this guid with GetResource<T> for that respective resource.
		- Do not store pointers, not even shared_ptrs, in the component.

		2. Serializing
		- Store instance_guid as uint64_t.
		- Do not need to store type_guid.

		3. Deserializing
		- Similar to serializing, just read the uint64_t into xresource::instance_guid.
		- Do not need to "load" the asset into the component, just call GetResource<T> when needed.

		Note: I realise calling hashmap everytime the asset is accessed may be slower than caching the pointer in full_guid.m_instance.
		However, I believe there will be <2500 asset access calls / frame, so it's likely negligible enough for M1 at least.
		*/
#pragma once
#ifndef ASSET_MANAGER_HPP
#define ASSET_MANAGER_HPP

#include <string>
#include <unordered_map>
#include <memory>
#include <filesystem>
#include <vector>
#include <cstdint>
#include "ISystem.hpp"

#include "xresource_guid.h"
#include "nlohmann/json.hpp"
#include "entt/meta/meta.hpp"
#include "entt/meta/resolve.hpp"

#ifdef PLATFORM_ANDROID
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#endif


namespace EngineFramework {


	
	class AssetManagerSystem : public ISystem
	{
	public:
		// ---- lifecycle ----
		/**
		 * @brief Construct and set the global singleton pointer.
		 * @post  GetInstance() returns this object.
		 */
		AssetManagerSystem();
		~AssetManagerSystem();
		static AssetManagerSystem& GetInstance();

		/*
			Cross-platform reading of data from file.
		*/
		static std::vector<unsigned char> GetDataFromFile(const std::filesystem::path& path);


#ifdef PLATFORM_ANDROID
		static void SetAndroidAssetManager(void* android_asset_manager);
#endif
		void Initialize() override;
		void Update(float) override;
		void PostUpdate(float) override;
		

		// Add a resource to the resource map (store shared_ptr of the resource as meta_any)
		template<typename T>
		static void AddResource(const xresource::full_guid& guid, std::shared_ptr<T> resource)
		{
			resource_map[guid] = entt::meta_any{ std::move(resource) };
		}

		//Gets a shared_ptr to the resource.
		template<typename T>
		static std::shared_ptr<T> GetResource(const xresource::full_guid& guid)
		{
			auto it = resource_map.find(guid);
			if (it != resource_map.end())
			{
				//Ensure the requested type is the actual type of the data, otherwise it can cause undefined behaviour.
				auto actual_type = it->second.type().info();
				auto requested_type = entt::resolve<std::shared_ptr<T> >().info();
				if (actual_type == requested_type)
				{
					return it->second.cast<std::shared_ptr<T> >();
				}
			}
			return nullptr;
		}

		//Gets a vector of shared_ptr to the resource types. Each asset is accompanied with its instance guid.
		template<typename T>
		static std::vector<std::pair<xresource::instance_guid, std::shared_ptr<T>>> GetAllResourceOfType(const xresource::type_guid& guid)
		{
			std::vector<std::pair<xresource::instance_guid, std::shared_ptr<T>>> asset_vector{};
			//Iterate over all resources
			for (auto it = resource_map.begin(); it != resource_map.end(); it++)
			{
				//Get all resource matching the type.
				if (it->first.m_Type != guid) continue;
				//Ensure the requested type is the actual type of the data, otherwise it can cause undefined behaviour.
				entt::type_info actual_type = it->second.type().info();
				entt::type_info requested_type = entt::resolve<std::shared_ptr<T> >().info();
				if (actual_type == requested_type)
				{
					asset_vector.emplace_back(it->first.m_Instance, it->second.cast<std::shared_ptr<T> >());
				}
			}
			
			return asset_vector;
		}


		//Return if the resource_map contains the resource.
		static bool HasResource(const xresource::full_guid& guid)
		{
			return resource_map.find(guid) != resource_map.end();
		}

		/*
			Return full guid of an asset given its name and type_guid.
			Also adds <guid, name> mapping if not already done.
		*/
		static xresource::full_guid GetFullGuid(const xresource::type_guid& type_guid, std::string asset_name)
		{
			auto it = name_guid_map[type_guid].find(asset_name);
			//Name guid map is set during loading of the respective asset.
			if (it == name_guid_map[type_guid].end())
			{
				LOGE("%s %s", "[AssetManager] No Asset with this name found in name_guid_map", asset_name.c_str());
				return {};
			}
			return xresource::full_guid{ it->second, type_guid };
		}

		/*
			Returns the name of an asset given its instance_guid.
			Returns nothing if it can't be found.
		*/
		static std::string GetName(xresource::instance_guid instance_guid)
		{
			auto it = guid_name_map.find(instance_guid);
			if (it != guid_name_map.end())
			{
				return it->second;
			}
			return {};
		}

		/*
			String names for each asset type, so they are consistent across the entire program.
		*/
		static inline constexpr const char TEXTURE[]{ "texture" };
		static inline constexpr const char MESH[]{ "mesh" };
		static inline constexpr const char SHADER[]{ "shader" };
		static inline constexpr const char CUBEMAP[]{ "cubemap" };
		static inline constexpr const char AUDIO[]{ "audio" };
		static inline constexpr const char ARCHTYPE[9]{ "archtype" };

		/*
			Precomputed type guids.
		*/
		static inline constexpr xresource::type_guid TEXTURE_GUID{ TEXTURE };
		static inline constexpr xresource::type_guid MESH_GUID{ MESH };
		static inline constexpr xresource::type_guid SHADER_GUID{ SHADER };
		static inline constexpr xresource::type_guid CUBEMAP_GUID{ CUBEMAP };
		static inline constexpr xresource::type_guid AUDIO_GUID{ AUDIO };
		static inline constexpr xresource::type_guid ARCHTYPE_GUID{ ARCHTYPE };
	private:
		
		/*
			Reads the config file located at ASSET_CONFIG_FILE, and calls the respective asset function loaders for each folder/config path.
			Will write into the resource maps.
		*/
		static void LoadAllResources();

		//Delete a resource from the map, calling its destructor.
		static void RemoveResource(const xresource::full_guid& guid)
		{
			resource_map.erase(guid);
		}
		//Delete all resources from the map, calling their destructors.
		static void RemoveAllResources()
		{
			resource_map.clear();
		}

		/*
			Cross-platform.
			List files in the given directory.
			Does not list files in subdirectories or directory names.

			If an extension list is passed, only files matching these extensions will be returned.
		*/
		static std::vector<std::filesystem::path> ListFilesInDirectory(const std::filesystem::path& folder_path, const std::vector<std::string>& accepted_extensions = {});


		/*
			Reads name->guid mapping from the metadata file in the folder provided.
			If no such mapping exists, it creates a new guid and writes the mapping to the file (windows only.)
			For android platform, ensure all metadata files are set (i.e. game loads all assets and thus sets the mapping).

			Returns guids in the same order as the names passed in.
		*/
		static std::vector<xresource::instance_guid> GetGuidMapping(const std::vector<std::string>& names, const std::filesystem::path& folder_path);

		/*
			Sets the <guid, resource> <guid, name> <name, guid> maps.
			Requires asset folder path.
		*/
		template <typename T>
		static void SetMappings(const xresource::type_guid& type_guid, const std::vector<std::string>& name_arr, const std::vector<std::shared_ptr<T>>& resource_arr, const std::filesystem::path& folder_path);

		//==Asset Loaders==//
		static bool LoadTexture(const std::filesystem::path&);
		static bool LoadCubemap(const std::filesystem::path&);
		static bool LoadMesh(const std::filesystem::path&);
		static bool LoadShader(const std::filesystem::path&);
		static bool LoadAudio(const std::filesystem::path&);
		static bool LoadArchtype(const std::filesystem::path&);


		static inline AssetManagerSystem* s_instance = nullptr; // singleton pointer
#ifdef PLATFORM_ANDROID
		static inline AAssetManager* android_asset_manager = nullptr;
#endif
		// <resource_type, func> that maps a resource type to a function.
		static inline std::unordered_map<std::string, std::function<bool(const std::filesystem::path&)>> asset_loader_map{};

		// <full_guid, resource_type> that maps a guid to a resource.
		static inline std::unordered_map<xresource::full_guid, entt::meta_any> resource_map{};
		// This is used to get human-readable names from instance_guid, like for level editor usage.
		static inline std::unordered_map<xresource::instance_guid, std::string> guid_name_map{};
		// This is used to get guids from human-readable names, like for anything "hardcoded", e.g. using a "Basic" shader. 
		// Note that type_guid is requested jic there are different asset types with the same name.
		// uint64_t is used for type_guid to allow hashing for unordered_map.
		// <type, <string, guid>>
		static inline std::unordered_map<xresource::type_guid, std::unordered_map<std::string, xresource::instance_guid>> name_guid_map{};

	};

} // namespace EngineFramework
#endif // ASSET_MANAGER_HPP
