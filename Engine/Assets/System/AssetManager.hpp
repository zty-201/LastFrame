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
		Resources are loaded at initialise time, and are each assigned a guid consisting of
		<name_guid><type_guid>.
		This guid will stay consistent as long as the type and name of the asset stays constant.

		To load an asset, a loader function bool(const std::filesystem::path&) is added to a <name, func> asset_loader_map in Initialize().
		This name must match ASSET_CONFIG_FILE defined in global.hpp.

		During Initialization, ASSET_CONFIG_FILE is read which gives <name, folderpath> json pairs. For each pair, the respective function loader is called
		and passed a folderpath.

		This loader function takes in a folderpath, and loads all assets of <T> inside. It may choose to use a separate config file within the folder, or load all assets as is.
		It generates a type_guid for all assets of <T> as well as an individual instance_guid for that asset name, before combining them
		into a full_guid.
		The loaded assets of type std::shared_ptr<T> is then added to the resource_map via AddResource, together with its full_guid.
		Upon any failure, it is to return false. Otherwise if all assets are loaded, return true.

		instance_guid --> dependent on asset name
		type_guid --> dependent on asset type name
		full_guid --> consists of instance and type guid.

		Usage==
		0. Renaming Asset
		- Note: Asset path should stay in the same folder.
		- If renaming an asset, ensure guid_to_name map is changed. Can use instance_id{"old_name"} to get the key-val pair, then change the name.
		- This is used for serialization.
		- Do not rename an asset if

		1. Accessing Asset
		- For every component, all asset types like texture, shader, mesh, audio file etc. are stored as xresource::full_guid.
		- Use this guid with GetResource<T> for that respective resource.
		- Do not store pointers, not even shared_ptrs, in the component.

		2. Serializing
		- For each asset, find the asset name from the full_guid using guid_to_name[full_guid.m_instance].
		- Convert the name to instance_guid.
		- Doing it this roundabout way allows it to consider asset renaming while the program is running.
		- Store the instance_guid as uint64_t.

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
			xresource::full_guid return_val;
			//Hash the asset name into a guid.
			return_val.m_Instance = xresource::instance_guid::GenerateGUIDCopy(asset_name.c_str());
			return_val.m_Type = type_guid;

			//If guid, name pair is not in map then add it in.
			if (guid_name_map.find(return_val.m_Instance) == guid_name_map.end())
			{
				guid_name_map[return_val.m_Instance] = asset_name;
			}

			return return_val;
		}

		/*
			String names for each asset type, so they are consistent across the entire program.
		*/
		static inline constexpr const char TEXTURE[]{ "texture" };
		static inline constexpr const char MESH[]{ "mesh" };
		static inline constexpr const char SHADER[]{ "shader" };
		static inline constexpr const char CUBEMAP[]{ "cubemap" };
		static inline constexpr const char AUDIO[]{ "audio" };

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

		//==Asset Loaders==//
		static bool LoadTexture(const std::filesystem::path&);
		static bool LoadCubemap(const std::filesystem::path&);
		static bool LoadMesh(const std::filesystem::path&);
		static bool LoadShader(const std::filesystem::path&);
		static bool LoadAudio(const std::filesystem::path&);


		static inline AssetManagerSystem* s_instance = nullptr; // singleton pointer
#ifdef PLATFORM_ANDROID
		static inline AAssetManager* android_asset_manager = nullptr;
#endif
		// <resource_type, func> that maps a resource type to a function. The function loads all assets within the directory, and sets guid_name_map as well.
		static inline std::unordered_map<std::string, std::function<bool(const std::filesystem::path&)>> asset_loader_map{};
		// <guid, name> map. Before serialization, asset instance_guid handles are converted to name, then the name is hashed back to instance_guid. 
		// This accounts for name changes during run-time (any asset name changes just update this map).
		static inline std::unordered_map<xresource::instance_guid, std::string> guid_name_map{};
		// <full_guid, resource_type> that maps a guid to a resource.
		static inline std::unordered_map<xresource::full_guid, entt::meta_any> resource_map{};


		/*
			During initialization,
			1. Assets are loaded via their respective folders into their data structures (stored in asset manager).
			2. Each type has its own 64bit guid.
			3. Each asset has their guid, based on <name 64bit guid><64bit type guid> --> create a <guid, name> map for serialization.
			4. During deserialization, components reference assets using <name><type>, which can be hashed into a guid.
			5. During run-time, assets are accessed from the asset manager via the guid.
			6. During serialization, guid is converted back into name.

			7. For renaming of assets, simply change the name in <guid, name> map. When serializing back, the guid will convert back to this name.
		*/
	};



	//    class AudioSystem;
	//    using AssetID = std::uint32_t;
	//
	//    /**
	//     * @brief Main path-only asset manager system.
	//     * @details Works as an engine system (ISystem) but its update hooks are no-ops.
	//     */
	//    class AssetManagerSystem : public ISystem {
	//    public:
	//        // ---- lifecycle ----
	//        /**
	//         * @brief Construct and set the global singleton pointer.
	//         * @post  GetInstance() returns this object.
	//         */
	//        AssetManagerSystem();
	//        static AssetManagerSystem& GetInstance();
	//
	//        /*
	//            Cross-platform reading of data from file.
	//        */
	//        static std::vector<unsigned char> GetDataFromFile(const std::filesystem::path& path);
	//
	//#ifdef PLATFORM_ANDROID
	//        static void SetAndroidAssetManager(void* android_asset_manager);
	//#endif
	//        void Initialize() override {}
	//        void Update(float) override {}
	//        void PostUpdate(float) override {}
	//
	//        // ---- configuration ----
	//        /**
	//         * @brief Set the root directory under which all asset paths live.
	//         * @param root Example: "Extern" or "Assets".
	//         * @note The directory is created if it does not exist.
	//         */
	//        void SetRoot(const std::filesystem::path& root);
	//
	//        // @brief Get the current root directory.
	//        [[nodiscard]] const std::filesystem::path& GetRoot() const { return m_root; }
	//
	//        // ---- registration ----
	//        /**
	//         * @brief Register a new asset by name and relative path.
	//         * @param name          Unique key you will use in code/editor.
	//         * @param relativePath  Path relative to the root directory.
	//         * @param verifyExists  If true, checks the file exists (non-blocking).
	//         * @return A new @ref AssetID handle.
	//         * @note Registration succeeds even if the file is missing (useful before import).
	//         */
	//        AssetID Register(const std::string& name,
	//                        const std::filesystem::path& relativePath,
	//                        bool verifyExists = true);
	//
	//        /**
	//         * @brief Remove an asset record by name (does not delete the file).
	//         */
	//        void    Unregister(const std::string& name);
	//
	//        /**
	//         * @brief Check whether an asset name is already registered.
	//         */
	//        bool    Has(const std::string& name) const;
	//
	//        /**
	//         * @brief Find the numeric id for a name.
	//         * @return The id or 0 if not found.
	//         */
	//        [[nodiscard]] AssetID FindId(const std::string& name) const;
	//
	//        // ---- path queries ----
	//        /**
	//         * @brief Get the stored relative path for an id.
	//         * @return Empty path if id is unknown.
	//         */
	//        [[nodiscard]] std::filesystem::path GetRelativePath(AssetID id) const;
	//
	//        /**
	//         * @brief Get the absolute path on disk for an id.
	//         * @return Empty path if id is unknown.
	//         */
	//        [[nodiscard]] std::filesystem::path GetAbsolutePath(AssetID id) const;
	//
	//        /**
	//         * @brief Check if the file currently exists on disk (by id).
	//         */
	//        [[nodiscard]] bool ExistsOnDisk(AssetID id) const;
	//
	//        /**
	//         * @brief Check if the file currently exists on disk (by name).
	//         */
	//        [[nodiscard]] bool ExistsOnDiskByName(const std::string& name) const;
	//
	//        // ---- relocation / renaming ----
	//        /**
	//         * @brief How to handle the file when changing the recorded path.
	//         */
	//        enum class MovePolicy {
	//            UpdateRecordOnly,   // Only change the stored path; do not touch the file.
	//            MoveOnDisk,         // Physically move/rename the file inside the root.
	//            CopyOnDisk          // Copy the file to the new location inside the root.
	//        };
	//
	//        /**
	//         * @brief Change the path of an asset.
	//         * @param name            Asset name to modify.
	//         * @param newRelativePath New path (relative to root).
	//         * @param policy          Choose whether to move/copy the file or only update the record.
	//         * @param allowOverwrite  If true, overwrites an existing destination file.
	//         * @return True on success.
	//         * @warning The destination must stay inside the root folder.
	//         */
	//        bool RelocateByName(const std::string& name,
	//                            const std::filesystem::path& newRelativePath,
	//                            MovePolicy policy = MovePolicy::MoveOnDisk,
	//                            bool allowOverwrite = false);
	//
	//        /**
	//         * @brief Rename the asset's *name* (does not change the file).
	//         * @return False if the new name already exists.
	//         */
	//        bool RenameName(const std::string& oldName, const std::string& newName);
	//
	//        // ---- discovery ----
	//        /**
	//         * @brief Import files from a folder by extension.
	//         * @param relativeDir Directory under root to scan.
	//         * @param extensions  Extensions to accept (e.g., {".png",".obj"}). Empty = all files.
	//         * @param recursive   If true, scans subfolders.
	//         * @return Number of new assets registered.
	//         */
	//        std::size_t ImportFolder(const std::filesystem::path& relativeDir,
	//                                const std::vector<std::string>& extensions,
	//                                bool recursive = true);
	//
	//        /**
	//         * @brief Get a vector of all (name, relativePath) pairs.
	//         */
	//        [[nodiscard]] std::vector<std::pair<std::string, std::filesystem::path>> List() const;
	//
	//        void SetAudioSystem(AudioSystem* audio);
	//
	//        bool LoadAudio(const std::string& id, const std::string& nameOrRel, bool loop = false);
	//
	//    private:
	//        struct Entry {
	//            AssetID id = 0;                                 // Numeric handle
	//            std::string name;                               // Asset name key
	//            std::filesystem::path relative;                 // Path relative to root
	//            std::filesystem::file_time_type lastWrite{};    // Last write (optional)
	//        };
	//
	//        // helpers
	//        // Build an absolute path from the stored relative path.
	//        [[nodiscard]] std::filesystem::path full(const std::filesystem::path& rel) const;
	//
	//        // Check that an absolute path stays inside the root.
	//        [[nodiscard]] bool insideRoot(const std::filesystem::path& abs) const;
	//
	//        // Convert string to lowercase (helper for extensions).
	//        [[nodiscard]] static std::string toLower(std::string s);
	//
	//        // data
	//        std::filesystem::path m_root{ "." };                // Root directory.
	//        AssetID m_next{ 1 };                                // Next id to hand out.
	//        std::unordered_map<std::string, Entry> m_byName;    // name to entry
	//        std::unordered_map<AssetID, std::string>    m_byId; // id to name
	//        static inline AssetManagerSystem* s_instance = nullptr; // singleton pointer
	//
	//        AudioSystem* m_audioSystem = nullptr;
	//
	//        std::string m_audioSubdir = "Audio";
	//#ifdef PLATFORM_ANDROID
	//        static inline AAssetManager* android_asset_manager = nullptr;
	//#endif

} // namespace EngineFramework
#endif // ASSET_MANAGER_HPP
