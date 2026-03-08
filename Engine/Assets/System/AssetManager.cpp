/**
 * @file    AssetManager.cpp
 * @author  Mohamed Arshad Bin Mohamed Hussain (mohamedarshad.b)
 * @brief   As of 14 September 2025, implement Path-only Asset Manager (no file parsing).
 *
 * @detail  This file contains only path / filesystem operations. No file I/O for parsing/decoding
 *          assets is performed here.
*/
#include "pch.hpp"
#include "AssetManager.hpp"
#include "Audio.hpp"
#include "Scripting/ScriptAsset.hpp" //For script asset type registration.
#include <system_error>
#include <mutex>

//For guid metadata file reading.
#ifdef PLATFORM_WINDOWS
#include <fstream>
#include <iomanip>
#endif

//Asset structs
#include "scene.h"
#include "ShaderProgram.h"
#include "Audio.hpp"
#include "SoundAsset.hpp"
#include "Archtype.hpp"

#include <mutex>

namespace EngineFramework {

	// -------- lifecycle --------
	AssetManagerSystem::AssetManagerSystem() {
		s_instance = this;
		/*
			Check for location of asset folder, prioritising the top-level folder first.
			This is because running from VS uses the current VS build folder, executable uses the current executable folder.
			writing to asset folder requires top-level asset folder whilst packaging executable requires it to use its current folder.
			Thus, always try to find a viable asset folder starting from the top level.
			Only windows needs to do this, android can use current folder as assets are stored in a fixed folder for apk.
		*/
#ifdef PLATFORM_WINDOWS
		std::filesystem::path arr[4]{ "../../../", "../../", "../", "" };
		for (const std::filesystem::path& prefix : arr)
		{
			//Look for extern folder
			std::filesystem::path asset_folder = prefix / "Extern";
			if (std::filesystem::is_directory(asset_folder))
			{
				extern_folder_prefix = prefix;
				LOGD("Using %s as Extern folder", asset_folder.string().c_str());
				break;
			}
		}
#endif
	}
	AssetManagerSystem::~AssetManagerSystem() { RemoveAllResources(); } //Call destructors on all resources.
	AssetManagerSystem& AssetManagerSystem::GetInstance() { return *s_instance; }

#ifdef PLATFORM_ANDROID
	void AssetManagerSystem::SetAndroidAssetManager(void* asset_manager)
	{
		android_asset_manager = static_cast<AAssetManager*>(asset_manager);
	}
#endif

	/*
			Cross-platform reading of data from file.
		*/
	std::vector<unsigned char> AssetManagerSystem::GetDataFromFile(const std::filesystem::path& path)
	{
#ifdef PLATFORM_ANDROID

		std::vector<unsigned char> buffer{};
		if (!android_asset_manager)
		{
			throw std::invalid_argument("AAssetManager is not initialized in AssetManager");
		}

		AAsset* asset = AAssetManager_open(android_asset_manager, path.c_str(), AASSET_MODE_BUFFER);
		if (!asset)
		{
			throw std::runtime_error("Failed to open asset: " + path.string());
		}

		/*
			Get Asset struct from AAssetManager, and read it into the buffer.
		*/

		off_t size = AAsset_getLength(asset);
		buffer.resize(size);

		int bytesRead = AAsset_read(asset, buffer.data(), size);
		if (bytesRead != size)
		{
			AAsset_close(asset);
			throw std::runtime_error("Failed to read entire asset: " + path.string());
		}

		AAsset_close(asset);
		return buffer;
#endif
#ifdef PLATFORM_WINDOWS
		//Check if path already has folder prefix.
		std::string prefix = GetAssetFolderPrefix().string();
		std::string path_string = path.string();
		//Not present, so append it.
		if (path_string.find(prefix) != 0)
		{
			path_string = prefix + path_string;
		}
		std::filesystem::path filepath{ path_string };
		// open the file:
		std::ifstream file(filepath, std::ios::binary);
		if (!file.is_open())
		{
			throw std::runtime_error("Failed to open asset: " + filepath.string());
		}
		// read the data:
		return std::vector<unsigned char>((std::istreambuf_iterator<char>(file)),
			std::istreambuf_iterator<char>());
#endif
	}
	/*
			Cross-platform reading of data from file.
		*/
	std::string AssetManagerSystem::GetStringFromFile(const std::filesystem::path& path)
	{
#ifdef PLATFORM_ANDROID

		std::string buffer{};
		if (!android_asset_manager)
		{
			throw std::invalid_argument("AAssetManager is not initialized in AssetManager");
		}

		AAsset* asset = AAssetManager_open(android_asset_manager, path.c_str(), AASSET_MODE_BUFFER);
		if (!asset)
		{
			throw std::runtime_error("Failed to open asset: " + path.string());
		}

		/*
			Get Asset struct from AAssetManager, and read it into the buffer.
		*/

		off_t size = AAsset_getLength(asset);
		buffer.resize(size);

		int bytesRead = AAsset_read(asset, buffer.data(), size);
		if (bytesRead != size)
		{
			AAsset_close(asset);
			throw std::runtime_error("Failed to read entire asset: " + path.string());
		}

		AAsset_close(asset);
		return buffer;
#endif
#ifdef PLATFORM_WINDOWS
		//Check if path already has folder prefix.
		std::string prefix = GetAssetFolderPrefix().string();
		std::string path_string = path.string();
		//Not present, so append it.
		if (path_string.find(prefix) != 0)
		{
			path_string = prefix + path_string;
		}
		std::filesystem::path filepath{ path_string };
		// open the file:
		std::ifstream file(filepath, std::ios::binary);
		if (!file.is_open())
		{
			throw std::runtime_error("Failed to open asset: " + filepath.string());
		}
		// read the data:
		return std::string((std::istreambuf_iterator<char>(file)),
			std::istreambuf_iterator<char>());
#endif
	}
	/*
			Cross-platform.
			List files in the given directory.
			Does not list files in subdirectories or directory names.

			If an extension list is passed, only files matching these extensions will be returned.
		*/
	std::vector<std::filesystem::path> AssetManagerSystem::ListFilesInDirectory(
		const std::filesystem::path& folder_path,
		const std::vector<std::string>& accepted_extensions)
	{
		std::vector<std::filesystem::path> file_list;

#ifdef PLATFORM_ANDROID
		// Use AAssetManager on Android
		if (!android_asset_manager)
		{
			throw std::runtime_error("Android Asset Manager not set for AssetManagerSystem before calling ListFilesInDirectory()");
		}

		AAssetDir* assetDir = AAssetManager_openDir(android_asset_manager, folder_path.string().c_str());
		//Check if any file is found in directory / if directory is valid.
		if (!assetDir)
			return file_list;

		const char* filename = nullptr;
		//Iterate through all files in directory until finished.
		while ((filename = AAssetDir_getNextFileName(assetDir)) != nullptr) {
			std::filesystem::path file_path = folder_path / filename;
			std::string ext = file_path.extension().string();
			//Skip any subdirectories or files with no extensions.
			if (ext.empty()) continue;

			// Check extensions to see if it matches the accepted extension list.
			//If no accepted extensions passed, accept all files.
			if (accepted_extensions.empty()) {
				file_list.push_back(file_path);
			}
			else {
				if (std::find(accepted_extensions.begin(), accepted_extensions.end(), ext) != accepted_extensions.end()) {
					file_list.push_back(file_path);
				}
			}
		}
		AAssetDir_close(assetDir);

#else
		std::filesystem::path asset_folder_path = GetAssetFolderPrefix() / folder_path;
		// Use std::filesystem on Windows / Linux
		if (std::filesystem::exists(asset_folder_path) && std::filesystem::is_directory(asset_folder_path)) {
			for (auto& entry : std::filesystem::directory_iterator(asset_folder_path)) {
				//Only check for files
				if (!entry.is_regular_file()) continue;
				auto ext = entry.path().extension().string();
				//Ensure file matches extension list. If none provided, accept all files.
				if (accepted_extensions.empty() ||
					std::find(accepted_extensions.begin(), accepted_extensions.end(), ext) != accepted_extensions.end()) {
					file_list.push_back(entry.path());
				}
			}
		}
#endif

		return file_list;
	}

	//Return if the resource_map contains the resource.
	bool AssetManagerSystem::HasResource(const xresource::full_guid& guid)
	{
		return resource_map.find(guid) != resource_map.end();
	}

	/*
			Return full guid of an asset given its name and type_guid.
		*/
	xresource::full_guid AssetManagerSystem::GetFullGuid(const xresource::type_guid& type_guid, std::string asset_name)
	{
		auto it = name_guid_map[type_guid].find(asset_name);
		//Name guid map is set during loading of the respective asset.
		if (it == name_guid_map[type_guid].end())
		{
			LOGI("%s %s", "[AssetManager] No Asset with this name found in name_guid_map", asset_name.c_str());
			return {};
		}
		return xresource::full_guid{ it->second, type_guid };
	}

	/*
			Returns the name of an asset given its instance_guid.
			Returns nothing if it can't be found.
		*/
	std::string AssetManagerSystem::GetName(xresource::full_guid asset_guid)
	{
		auto it = guid_name_map.find(asset_guid);
		if (it != guid_name_map.end())
		{
			return it->second;
		}
		return {};
	}

	/*
			Reads name->guid mapping from the metadata file in the folder provided.
			If no such mapping exists, it creates a new guid and writes the mapping to the file (windows only.)
			For android platform, ensure all metadata files are set (i.e. game loads all assets and thus sets the mapping).

			Returns guids in the same order as the names passed in.
		*/
	std::vector<xresource::instance_guid> AssetManagerSystem::GetGuidMapping(const std::vector<std::string>& names, const std::filesystem::path& folder_path)
	{
		//Decide if the json file needs to be modified.
		bool isNewMapping = false;
		std::vector<xresource::instance_guid> guid_arr{};
		std::string guid_meta_string;
		std::filesystem::path metadata_file = folder_path / "guid.meta";

		/*
			1. Read metadata file, if it doesn't exist then throw an error.

			Note: If it doesn't exist, simply create a file "guid.meta" in the asset folder that has { } written in it.
		*/
		try
		{
			guid_meta_string = GetStringFromFile(metadata_file);
		}
		catch (std::exception&)
		{
			throw std::runtime_error((std::string("[AssetManager] Unable to find guid.meta in: ") + folder_path.string()).c_str());
		}
		nlohmann::json guid_meta_json = nlohmann::json::parse(guid_meta_string);

		/*
			Find name mappings.
		*/
		for (const std::string& name : names)
		{
			//Add name mapping to array.
			if (guid_meta_json.contains(name))
			{
				guid_arr.emplace_back(guid_meta_json[name].get<std::uint64_t>());
				continue;
			}
			//Name mapping doesn't exist in metadata file, so add it in.
			//This should not occur in android, as all asset and metadata files must be set beforehand (i.e. run game on desktop first).
#ifdef PLATFORM_WINDOWS
			xresource::instance_guid generated_guid = xresource::instance_guid::GenerateGUIDCopy();
			guid_meta_json[name] = generated_guid.m_Value;
			guid_arr.emplace_back(generated_guid);
			isNewMapping = true;
#else
			throw std::runtime_error("Metadata file has not been fully configured, please load all assets on desktop to set metadata before compiling android");
#endif
		}

		/*
			Write back to file if necessary (e.g. new mappings generated).
			Only done for windows, should be unnecessary for android.
		*/
#ifdef PLATFORM_WINDOWS
		if (isNewMapping)
		{
			std::ofstream metadata_out{ GetAssetFolderPrefix() / metadata_file , std::ios::trunc };
			metadata_out << std::setw(4) << guid_meta_json << std::endl;
			metadata_out.close();
		}
#endif

		return guid_arr;
	}

	/*
			Sets the <guid, resource> <guid, name> <name, guid> maps.
			Requires asset folder path.
		*/
	template <typename T>
	void AssetManagerSystem::SetMappings(const xresource::type_guid& type_guid, const std::vector<std::string>& name_arr, const std::vector<std::shared_ptr<T>>& resource_arr, const std::filesystem::path& folder_path)
	{
		if (name_arr.size() != resource_arr.size())
		{
			throw std::logic_error("[AssetManager] Number of resource names should match number of loaded resources, was there any unloaded resources whose names were added?");
		}

		/*
			First, get the guids mapped from name.
		*/
		std::vector<xresource::instance_guid> guid_mapping = GetGuidMapping(name_arr, folder_path);
		if (guid_mapping.size() != resource_arr.size())
		{
			throw std::logic_error("[AssetManager] Number of guids should match number of loaded resources");
		}
		for (int i = 0; i < resource_arr.size(); i++)
		{
			xresource::instance_guid& instance_guid = guid_mapping[i];
			xresource::full_guid asset_guid{ instance_guid, type_guid };
			const std::string& name = name_arr[i];

			//	Add resource to the resource map.
			AddResource(asset_guid, resource_arr[i]);

			//Add <guid, name> and <name, guid> mapping.
			guid_name_map[asset_guid] = name;
			name_guid_map[type_guid][name] = instance_guid;
		}
	}

	void AssetManagerSystem::Initialize()
	{
		//==Add function loaders for each type==//
		asset_loader_map[TEXTURE] = std::function<bool(const std::filesystem::path&)>(&AssetManagerSystem::LoadTexture);
		asset_loader_map[CUBEMAP] = std::function<bool(const std::filesystem::path&)>(&AssetManagerSystem::LoadCubemap);
		asset_loader_map[ANIMATION] = std::function<bool(const std::filesystem::path&)>(&AssetManagerSystem::LoadAnimation);
		asset_loader_map[MESH] = std::function<bool(const std::filesystem::path&)>(&AssetManagerSystem::LoadMesh);
		asset_loader_map[SHADER] = std::function<bool(const std::filesystem::path&)>(&AssetManagerSystem::LoadShader);
		asset_loader_map[AUDIO] = std::function<bool(const std::filesystem::path&)>(&AssetManagerSystem::LoadAudio);
		asset_loader_map[SCRIPT] = std::function<bool(const std::filesystem::path&)>(&AssetManagerSystem::LoadScript);
		asset_loader_map[ARCHTYPE] = std::function<bool(const std::filesystem::path&)>(&AssetManagerSystem::LoadArchtype);
		//=====================================//

		LoadAllResources();
	}
	void AssetManagerSystem::Update(float)
	{

	}
	void AssetManagerSystem::PostUpdate(float)
	{

	}


	/*
		Reads the config file located at ASSET_CONFIG_FILE, and calls the respective asset function loaders for each folder/config path.
		Will write into the resource maps.
	*/
	void AssetManagerSystem::LoadAllResources()
	{
		/*
			Access each folder specified in the json file.
		*/
		std::string config_json_string = GetStringFromFile(std::filesystem::path(ASSET_CONFIG_FILE));
		nlohmann::json config_json = nlohmann::json::parse(config_json_string);

		/*
			Each resource type is located in their own folder.
			Iterate through each asset in that folder.
			For each asset, load using the respective loader (use resource_type to access the loader) and create a full_guid.

			Modify <guid, name> map for serialization.

			//TODO: Multithreading for asset loading here. Ensure no race conditions(like writing to the resource map) and multi-threaded safety within the function loaders too. (e.g. accessing the same file, writing to the same resource map).
		*/
		for (const auto& task : config_json)
		{
			std::string resource_type = task["type"];
			std::filesystem::path folder_path = task["path"].get<std::string>();

			// The rest of your logic remains the same...
			type_folderpath_map[resource_type] = folder_path;
			//Unable to find resource type in map, it might not have been added yet.
			if (asset_loader_map.find(resource_type) == asset_loader_map.end())
			{
				throw std::runtime_error((std::string("No Function Loader for resource type: ") + resource_type +
					". Ensure it is spelt correctly, and asset loader has been added to asset_loader_map.").c_str());
			}
			//Call the asset loader for that respective asset type, and pass in the folderpath.
			if (!asset_loader_map[resource_type](folder_path))
			{
				throw std::runtime_error((std::string("Unable to load asset of type: ") + resource_type).c_str());
			}
		}
	}


	//==Asset Loaders==//
	bool AssetManagerSystem::LoadTexture(const std::filesystem::path& folder_path)
	{
		/*
		* Note: Only one of each name is to be loaded, even if there are multiple extensions.
		
			Process:
			1a. Get list of all filepaths in compressed files directory (for the respective platform) and original directory. Store as dictionary and list separately.
			1b. Get list of all file names in do_not_compress.meta, store as dictionary.
			2. If file is not in compressed files directory dictionary AND it is not in the do_not_compress.meta file dictionary, then compress and store.
			2b. If compression failed, then add filename to do_not_compress.meta.
			3. Load compiled textures from respective compressed directory based on platform.
			- ETC2 for android platform
			- BCx for windows
			- Add name to array if successfully loaded.
			4. For each texture in original directory, load as uncompressed file IF AssetManagerSystem does not have it already loaded.
			- This also acts as a failsafe in case compressed files failed to load for some reason.
			- Add name to array if loaded.
			5. Because SetGUIDMappings relies on a name array and not filepath array, no need to worry about guid mapping.
			Just ensure only one of each name is in the array.
		*/

		LOGI("Loading Textures...");
		//Acceptable files that can be loaded as a texture.
		std::vector<std::string> texture_extensions{ ".png", ".dds", ".ktx", ".ktx2", ".DDS" };
		std::vector<std::filesystem::path> files = ListFilesInDirectory(folder_path, texture_extensions);

		/*
			Asset names used for mapping to guid.
			Resources are kept in an array until guid mappings are read.
		*/
		std::vector<std::string> asset_names{};
		std::vector<std::shared_ptr<Scene::Texture>> loaded_resources{};

		//Iterate through all texture files
		for (const std::filesystem::path& path : files)
		{
			std::string name = path.stem().string();
			//Allocate resource
			try
			{
				loaded_resources.emplace_back(new Scene::Texture(path.string()));
				asset_names.emplace_back(name); //Done if resource loading is a success.
				LOGI("Loaded Texture: %s", name.c_str());
			}
			catch (std::exception& e)
			{
				LOGE("%s %s %s", "Unable to Load Texture", name.c_str(), e.what());
			}
		}

		SetMappings(TEXTURE_GUID, asset_names, loaded_resources, folder_path);
		return true;
	}
	bool AssetManagerSystem::LoadCubemap(const std::filesystem::path& config_file)
	{
		LOGI("Loading Cubemaps...");

		/*
			Asset names used for mapping to guid.
			Resources are kept in an array until guid mappings are read.
		*/
		std::vector<std::string> asset_names{};
		std::vector< std::shared_ptr<Scene::Texture>> loaded_resources{};

		//Read the Config File
		std::string config_json_string = GetStringFromFile(config_file);
		nlohmann::json config_json = nlohmann::json::parse(config_json_string);

		//The config file should contain <name, filepath_arr> pairs
		for (auto& [cubemap_name, filepath_arr] : config_json.items())
		{
			try
			{
				loaded_resources.emplace_back(new Scene::Texture(filepath_arr.get<std::array<std::string, 6>>()));
				asset_names.emplace_back(cubemap_name); //Done if resource loading is a success.
				LOGI("Loaded Cubemap: %s", cubemap_name.c_str());
			}
			catch (std::exception& e)
			{
				LOGE("%s %s %s", "Unable to Load Cubemap", cubemap_name.c_str(), e.what());
			}
		}

		SetMappings(CUBEMAP_GUID, asset_names, loaded_resources, config_file.parent_path());

		return true;
	}

	bool AssetManagerSystem::LoadMesh(const std::filesystem::path& folder_path)
	{
		/*UNREFERENCED_PARAMETER(folder_path);
		LOGI("Mesh Loader not yet implemented.");*/
		LOGI("Loading Meshes...");

		// Acceptable files that can be loaded as a mesh.
		std::vector<std::string> mesh_extensions{ ".obj", ".fbx" };
		std::vector<std::filesystem::path> files = ListFilesInDirectory(folder_path, mesh_extensions);

		/*
			Asset names used for mapping to guid.
			Resources are kept in an array until guid mappings are read.
		*/
		std::vector<std::string> asset_names{};
		std::vector< std::shared_ptr<Mesh::MeshData>> loaded_resources{};

		std::vector<std::thread> thread_arr{};

		std::mutex num_pending_mutex;
		std::atomic<int> num_pending_gpu_load{}; //Keeps track of how many meshes need to be loaded into gpu mem
		std::vector<std::shared_ptr<Mesh::MeshData>> pending_gpu_load_arr{}; //Keep ptrs to do a final function call after join.

		/*
			Multi-Threading Guide
			1. Main thread runs a side-thread for each mesh load, and increments num_pending_gpu_load to show total number of active mesh loads pending gpu load.
			- Afterwards it sleeps and waits for work to come (gpu load).
			- This prevents spinlocking which slows other threads.
			2. Each thread completes the mesh load (excluding gpu load)
			- If mesh cannot be loaded, it should not be loaded to gpu so num_pending_gpu_load is decremented.
			- Otherwise, the side thread wakes up the main thread using condition variable, so it can load into gpu.
			3. Main thread loads into gpu and decrements num_pending_gpu_load.
			4. When num_pending_gpu_load == 0, that means all threads have completed.
		*/
		std::condition_variable gpu_cv;

		// Collect the names and loop through all mesh files
		for (const std::filesystem::path& path : files) {
			// Obtain the name of the mesh from the file name (without extension)
			std::string name = path.stem().string();
			auto create_append_thread = [path, &num_pending_mutex, &pending_gpu_load_arr, &num_pending_gpu_load, &gpu_cv, name] {
				// Try catch guard for loading mesh
				try {
					std::shared_ptr<Mesh::MeshData> mesh_data{ new Mesh::MeshData(path.string()) };
					mesh_data->name = name;
					//Lock-guard to prevent race conditions for multi-threading.
					{
						std::lock_guard<std::mutex> mesh_lock(num_pending_mutex);
						pending_gpu_load_arr.emplace_back(mesh_data); //Only add when mesh is fully prepared for gpu load.
					}
					//Do this outside locking so there's no risk of deadlock.
					gpu_cv.notify_one(); //Wake up the main thread to load into gpu.
				}
				catch (std::exception& e) {
					LOGE("%s %s %s", "Unable to Load Mesh", name.c_str(), e.what());
					num_pending_gpu_load--; //Invalid mesh, so no gpu load.
					if (num_pending_gpu_load == 0) gpu_cv.notify_one(); //Rare case where the last mesh cannot load, prevents main thread from sleeping forever.
				}

				};
			num_pending_gpu_load++; //Each mesh should gpu load after completion.
			thread_arr.emplace_back(create_append_thread);
		}
		//Main thread loads into gpu, side threads create mesh and append to pending_gpu_load_arr
		while(num_pending_gpu_load > 0)
		{
			std::shared_ptr<Mesh::MeshData> incomplete_mesh;
			//Check if a thread has finished so the mesh can be pushed to the gpu.
			{
				std::unique_lock<std::mutex> mesh_lock(num_pending_mutex);
				gpu_cv.wait(mesh_lock, [&pending_gpu_load_arr, &num_pending_gpu_load]{return (!pending_gpu_load_arr.empty() || num_pending_gpu_load == 0);}); //Wait if there's nothing to load right now.
				if (num_pending_gpu_load == 0) break; //The case where the last mesh cannot load, meaning there's nothing more to work on and the loop should break.
				incomplete_mesh = pending_gpu_load_arr.back(); 
				pending_gpu_load_arr.pop_back();
			}
			num_pending_gpu_load--; //Eat one.
			try {
				//No need for mutex since the other threads have completed their work on this mesh data already.
				incomplete_mesh->LoadWithFilename_AfterThreadJoinCall(); //Can only be done on main thread.
				loaded_resources.emplace_back(incomplete_mesh);
				asset_names.emplace_back(incomplete_mesh->name); // Done if resource loading is a success.
			}
			catch (std::exception& e) {
				LOGE("%s %s %s", "Unable to load Mesh into GPU memory", incomplete_mesh->name.c_str(), e.what());
			}

			LOGI("Loaded Mesh: %s  |  Vertex Count: %d  | Bone Count: %d", incomplete_mesh->name.c_str(), incomplete_mesh->num_vertices, incomplete_mesh->num_bones);
		}

		//Wait for all mesh loaders
		for (std::thread& thread : thread_arr)
		{
			thread.join();
		}
		
		// Set Mapping with mesh GUID
		SetMappings(MESH_GUID, asset_names, loaded_resources, folder_path);

		return true;
	}

	/**
	 * @brief Loads a shader source file and recursively processes all #include directives.
	 * @param filepath The path to the shader file to load.
	 * @return A string containing the full, combined shader source code.
	*/
	static std::string LoadShaderSourceWithIncludes(const std::filesystem::path& filepath) {
		// Use the AssetManager's own file reading utility to be platform-agnostic
		std::string full_source = AssetManagerSystem::GetStringFromFile(filepath);
		std::stringstream final_source;
		std::stringstream file_stream(full_source);

		std::string line;
		while (getline(file_stream, line)) {
			// Check for the #include directive
			if (line.rfind("#include", 0) == 0) {
				// Found an include, parse the filename (e.g., <common.glsl>)
				size_t start = line.find('<');
				size_t end = line.rfind('>');
				if (start == std::string::npos || end == std::string::npos || start >= end) {
					final_source << line << "\n"; // Malformed include, pass it along
					continue;
				}

				std::string include_path_str = line.substr(start + 1, end - start - 1);

				// Assume the included file is in the same directory as the parent shader
				std::filesystem::path include_filepath = filepath.parent_path() / include_path_str;

				// Recursively call this function to load the included file's source
				std::string include_source = LoadShaderSourceWithIncludes(include_filepath);

				if (!include_source.empty()) {
					final_source << include_source << "\n";
				}
				else {
					LOGE("Failed to include shader file: %s", include_filepath.string().c_str());
				}
			}
			else {
				// Not an include line, just append it to the final source
				final_source << line << "\n";
			}
		}

		return final_source.str();
	}

	bool AssetManagerSystem::LoadShader(const std::filesystem::path& config_file)
	{
		LOGI("Loading Shader");
		/*
			Asset names used for mapping to guid.
			Resources are kept in an array until guid mappings are read.
		*/
		std::vector<std::string> asset_names{};
		std::vector< std::shared_ptr<ShaderProgram>> loaded_resources{};

		//Read the Config File
		std::string config_json_string = GetStringFromFile(config_file);
		nlohmann::json config_json = nlohmann::json::parse(config_json_string);

		//The config file should contain <name, shader_filepaths> pairs
		for (auto& [shader_name, shader_filepaths] : config_json.items())
		{
			//Read vertex and fragment shader filepaths.
			//Always throw an error if not present, as these are critical for a shader program (unlike geometry or tesselation).
			try
			{
				// Case 1: Compute Shader (has a "compute" key)
				if (shader_filepaths.contains("compute"))
				{
					std::string compute_shader_path = shader_filepaths["compute"];
					std::string compute_source = LoadShaderSourceWithIncludes(compute_shader_path);
					if (compute_source.empty()) throw std::runtime_error("Failed to load compute shader from " + compute_shader_path);

					loaded_resources.emplace_back(new ShaderProgram(compute_source.c_str()));
					asset_names.emplace_back(shader_name);
				}
				// Case 2: Graphics Shader (has "vertex" and "fragment" keys)
				else if (shader_filepaths.contains("vertex") && shader_filepaths.contains("fragment"))
				{
					std::string vertex_shader_path = shader_filepaths["vertex"];
					std::string fragment_shader_path = shader_filepaths["fragment"];

					std::string vertex_source = LoadShaderSourceWithIncludes(vertex_shader_path);
					std::string fragment_source = LoadShaderSourceWithIncludes(fragment_shader_path);
					if (vertex_source.empty() || fragment_source.empty()) throw std::runtime_error("Failed to load graphics shaders from " + vertex_shader_path + " or " + fragment_shader_path);

					// Call the new constructor that takes source code
					loaded_resources.emplace_back(new ShaderProgram(vertex_source.c_str(), fragment_source.c_str(), true));
					asset_names.emplace_back(shader_name);
				}
				else
				{
					LOGE("Shader '%s' in config is missing required keys. Expected 'compute' or both 'vertex' and 'fragment'.", shader_name.c_str());
					continue;
				}
			}
			catch (std::exception& e)
			{
				LOGE("%s %s %s", "Unable to Load Shader", shader_name.c_str(), e.what());
			}
		}

		SetMappings(SHADER_GUID, asset_names, loaded_resources, config_file.parent_path());

		return true;
	}
	bool AssetManagerSystem::LoadAudio(const std::filesystem::path& config_file)
	{
		LOGI("Loading Audio");
		/*
			Asset names used for mapping to guid.
			Resources are kept in an array until guid mappings are read.
		*/
		std::vector<std::string> asset_names{};
		std::vector< std::shared_ptr<SoundAsset>> loaded_resources{};
		/*
			There are two ways to load audio
			1. Specified in Config File: Allows audio to be loaded in loop or non-loop modes.
			2. Not Specified in Config File: Loads audio not in config file as non-looped.
		*/

		//Read the Config File
		std::string config_json_string = GetStringFromFile(config_file);
		nlohmann::json config_json = nlohmann::json::parse(config_json_string);

		//Reading all filepaths for audio files in the current directory.
		std::filesystem::path folder_path = config_file.parent_path();
		//Acceptable files that can be loaded as an audio asset.
		std::vector<std::string> audio_extensions{ ".mp3", ".wav" };
		std::vector<std::filesystem::path> files = ListFilesInDirectory(folder_path, audio_extensions);

		//Iterate through all audio files in config file.
		for (auto& [audio_name, config] : config_json.items())
		{
			if (!config.contains("filepath"))
			{
				LOGE("%s %s %s", "No filepath in config file for audio file", audio_name.c_str(), ", did you mispell it ?");
				continue;
			}
			if (!config.contains("isLoop"))
			{
				LOGE("%s %s %s", "No loop variable set in config file for audio file", audio_name.c_str(), ", did you mispell it ?");
				continue;
			}

			try
			{
				loaded_resources.emplace_back(AudioSystem::GetInstance().Load((GetAssetFolderPrefix().string() + config["filepath"].get<std::string>()).c_str(), config["isLoop"]));
				asset_names.emplace_back(audio_name); //Done if resource loading is a success.
				//if resource was loaded, then remove it from the array so it doesn't get duplicate loaded later on.
				for (auto it = files.begin(); it != files.end(); it++)
				{
					//Stem both filepaths, in order to do a filename comparison (and not a filename to json_name comparison).
					//Use this instead of equivalent, to prevent it trying to read a path (that is not there on android).
					if (it->stem() == std::filesystem::path(config["filepath"].get<std::string>()).stem())
					{
						files.erase(it);
						break;
					}
				}
			}
			catch (std::exception& e)
			{
				LOGE("%s %s %s", "Unable to Load Audio File listed in Config file", audio_name.c_str(), e.what());
			}
		}
		/*
				All audio files listed in config file has been loaded.
				Load the rest not specified in config file.
			*/
		for (const std::filesystem::path& audio_path : files)
		{
			//Get guid (and update guid_name_map by calling GetFullGuid.
			std::string audio_name = audio_path.stem().string();
			try
			{
				loaded_resources.emplace_back(AudioSystem::GetInstance().Load(audio_path.string(), false));
				asset_names.emplace_back(audio_name); //Done if resource loading is a success.
			}
			catch (std::exception& e)
			{
				LOGE("%s %s %s", "Unable to Load Audio File", audio_name.c_str(), e.what());
			}
		}

		SetMappings(AUDIO_GUID, asset_names, loaded_resources, folder_path);

		return true;
	}

	bool AssetManagerSystem::LoadScript(const std::filesystem::path& folder_path)
	{
		LOGD("Loading Script");
		//Acceptable files that can be loaded as a texture.
		std::vector<std::string> script_extensions{ ".lua" };
		std::vector<std::filesystem::path> files = ListFilesInDirectory(folder_path, script_extensions);

		/*
			Asset names used for mapping to guid.
			Resources are kept in an array until guid mappings are read.
		*/
		std::vector<std::string> asset_names{};
		std::vector< std::shared_ptr<ScriptAsset>> loaded_resources{};

		//Iterate through all texture files
		for (const std::filesystem::path& path : files)
		{
			std::string name = path.stem().string();
			//Allocate resource
			try
			{
				auto script = std::make_shared<ScriptAsset>();
				script->relPath = path;

				// Read Lua code once at load time (works on both Windows/Android)
				auto bytes = GetDataFromFile(path);
				script->code = std::string(bytes.begin(), bytes.end());

				loaded_resources.emplace_back(script);
				asset_names.emplace_back(name);
			}
			catch (std::exception& e)
			{
				LOGE("%s %s %s", "Unable to Load Script", name.c_str(), e.what());
			}
		}

		SetMappings(SCRIPT_GUID, asset_names, loaded_resources, folder_path);

		return true;
	}

	bool AssetManagerSystem::LoadArchtype(const std::filesystem::path& folder_path)
	{
		LOGI("Loading Archtype");
		/*
			Asset names used for mapping to guid.
			Resources are kept in an array until guid mappings are read.
		*/
		std::vector<std::string> asset_names{};
		std::vector<std::shared_ptr<Archtype>> loaded_resources{};
		//Acceptable files that can be loaded as a texture.
		std::vector<std::string> archtype_extensions{ ".json" };

		std::vector<std::filesystem::path> files = ListFilesInDirectory(folder_path, archtype_extensions);

		//Iterate through all archtype files
		for (const std::filesystem::path& path : files)
		{
			//Get guid (and update guid_name_map by calling GetFullGuid).
			std::string name = path.stem().string();

			//Allocate resource
			try
			{
				std::string json_string = GetStringFromFile(path);
				loaded_resources.emplace_back(new Archtype(json_string));
				asset_names.emplace_back(name); //Done if resource loading is a success.
			}
			catch (std::exception& e)
			{
				LOGE("%s %s %s", "Unable to Load Archtype file", name.c_str(), e.what());
			}
		}

		SetMappings(ARCHTYPE_GUID, asset_names, loaded_resources, folder_path);

		return true;
	}

	bool AssetManagerSystem::LoadAnimation(const std::filesystem::path& folder_path)
	{
		LOGD("Loading Animation");
		//Acceptable files that can be loaded as a texture.
		std::vector<std::string> animation_extensions{ ".fbx" };
		std::vector<std::filesystem::path> files = ListFilesInDirectory(folder_path, animation_extensions);

		/*
			Asset names used for mapping to guid.
			Resources are kept in an array until guid mappings are read.
		*/
		std::vector<std::string> asset_names{};
		std::vector< std::shared_ptr<animImport>> loaded_resources{};

		//Iterate through all texture files
		for (const std::filesystem::path& path : files)
		{
			std::string name = path.stem().string();
			//Allocate resource
			try
			{
				loaded_resources.emplace_back(new animImport(OFBXAnimationImporter::importFromFile(path.string())));
				asset_names.emplace_back(name); //Done if resource loading is a success.
			}
			catch (std::exception& e)
			{
				LOGE("%s %s %s", "Unable to Load Animation", name.c_str(), e.what());
			}
		}

		SetMappings(ANIMATION_GUID, asset_names, loaded_resources, folder_path);

		return true;
	}


	//Delete all resources from the map, calling their destructors.
	void AssetManagerSystem::RemoveAllResources()
	{
		resource_map.clear();
		guid_name_map.clear();
		name_guid_map.clear();
	}


	//Delete a resource from the map, calling its destructor.
	void AssetManagerSystem::RemoveResource(const xresource::full_guid& guid)
	{
		resource_map.erase(guid);
		guid_name_map.erase(guid);
		auto& map = name_guid_map[guid.m_Type];
		for (auto it = map.begin(); it != map.end(); it++)
		{
			if (it->second != guid.m_Instance) continue;
			map.erase(it);
			break;
		}
	}

	void AssetManagerSystem::ModifyNameMappings(xresource::type_guid type, const std::string& old_name, const std::string& new_name)
	{
		//Modify name_guid_map and guid_name_map
		xresource::full_guid asset_guid = GetFullGuid(type, old_name);
		guid_name_map[asset_guid] = new_name;
		//Add new entry and delete old entry.
		name_guid_map[type][new_name] = asset_guid.m_Instance;
		name_guid_map[type].erase(old_name);
	}

	std::filesystem::path AssetManagerSystem::GetFolderPath(const std::string& type)
	{
		auto result = type_folderpath_map.find(type);
		if (result != type_folderpath_map.end())
		{
			return result->second;
		}
		return {};
	}


	//Reloads all resource of a specific type, and calls destructors in the process.
	void AssetManagerSystem::ReloadAllResourcesOfType(std::string type, xresource::type_guid type_guid)
	{
		//==First ensure that type is a valid string, to prevent deletion of existing assets without loading them back.
		if (asset_loader_map.find(type) == asset_loader_map.end())
		{
			LOGE("No Function Loader for resource type %s", type.c_str());
			return;
		}
		std::string config_json_string = GetStringFromFile(std::filesystem::path(ASSET_CONFIG_FILE));
		nlohmann::json config_json = nlohmann::json::parse(config_json_string);

		std::string folderpath_string;
		for (auto& json_pair : config_json)
		{
			if (!json_pair.contains("type") || json_pair["type"] != type) continue;
			if (!json_pair.contains("path")) continue;
			folderpath_string = json_pair["path"];
			break;
		}

		if (folderpath_string.empty())
		{
			LOGE("Asset Manager ReloadAllResourceOfType: No such resource type or no folderpath for resource type %s in asset config file", type.c_str());
			return;
		}


		//==From each of the 3 maps, remove every asset corresponding to type.
		for (auto it = resource_map.begin(); it != resource_map.end(); )
		{
			if (it->first.m_Type != type_guid)
			{
				it++;
				continue;
			}
			it = resource_map.erase(it);
		}
		for (auto it = guid_name_map.begin(); it != guid_name_map.end(); )
		{
			if (it->first.m_Type != type_guid)
			{
				it++;
				continue;
			}
			it = guid_name_map.erase(it);
		}
		name_guid_map.erase(type_guid);

		//==Load again.
		std::filesystem::path folder_path{ folderpath_string };


		//Call the asset loader for that respective asset type, and pass in the folderpath.
		if (!asset_loader_map[type](folder_path))
		{
			LOGE("Unable to load assets of type: %s", type.c_str());
		}
	}

} // namespace EngineFramework

