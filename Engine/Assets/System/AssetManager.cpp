/**
 * @file    AssetManager.cpp
 * @author  Mohamed Arshad Bin Mohamed Hussain (mohamedarshad.b)
 * @brief   As of 14 September 2025, implement Path-only Asset Manager (no file parsing).
 *
 * @detail  This file contains only path / filesystem operations. No file I/O for parsing/decoding
 *          assets is performed here.
*/
#include "AssetManager.hpp"
#include "Audio.hpp"
#include <system_error>
#include <fstream>
#include <filesystem>
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
namespace EngineFramework {

	// -------- lifecycle --------
	AssetManagerSystem::AssetManagerSystem() { s_instance = this; }
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

		LOGD("Opening Asset: %s", path.c_str());
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
		// open the file:
		std::ifstream file(path, std::ios::binary);
		if (!file.is_open())
		{
			throw std::runtime_error("Failed to open asset: " + path.string());
		}
		// read the data:
		return std::vector<unsigned char>((std::istreambuf_iterator<char>(file)),
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
			throw std::exception("Android Asset Manager not set for AssetManagerSystem before calling ListFilesInDirectory()");
		}

		AAssetDir* assetDir = AAssetManager_openDir(android_asset_manager, folder_path.string().c_str());
		//Check if any file is found in directory / if directory is valid.
		if (!assetDir)
			return file_list;

		const char* filename = nullptr;
		//Iterate through all files in directory until finished.
		while ((filename = AAssetDir_getNextFileName(assetDir)) != nullptr) {
			std::filesystem::path file_path = folder_path / filename;

			// Check extensions to see if it matches the accepted extension list.
			//If no accepted extensions passed, accept all files.
			if (accepted_extensions.empty()) {
				file_list.push_back(file_path);
			}
			else {
				std::string ext = file_path.extension().string();
				if (std::find(accepted_extensions.begin(), accepted_extensions.end(), ext) != accepted_extensions.end()) {
					file_list.push_back(file_path);
				}
			}
		}
		AAssetDir_close(assetDir);

#else
		// Use std::filesystem on Windows / Linux
		if (std::filesystem::exists(folder_path) && std::filesystem::is_directory(folder_path)) {
			for (auto& entry : std::filesystem::directory_iterator(folder_path)) {
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
		std::vector<unsigned char> guid_meta_string;
		std::filesystem::path metadata_file = folder_path / "guid.meta";

		/*
			1. Read metadata file, if it doesn't exist then throw an error.

			Note: If it doesn't exist, simply create a file "guid.meta" in the asset folder that has { } written in it.
		*/
		try
		{
			guid_meta_string = GetDataFromFile(metadata_file);
		}
		catch (std::exception&)
		{
			throw std::exception((std::string("[AssetManager] Unable to find guid.meta in: ") + folder_path.string()).c_str());
		}
		std::string temp{ guid_meta_string.begin(), guid_meta_string.end() };
		nlohmann::json guid_meta_json = nlohmann::json::parse(temp);

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
			std::ofstream metadata_out{ metadata_file , std::ios::trunc };
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
			xresource::instance_guid &instance_guid = guid_mapping[i];
			const std::string &name = name_arr[i];

			//	Add resource to the resource map.
			AddResource(xresource::full_guid(instance_guid, type_guid), resource_arr[i]);

			//Add <guid, name> and <name, guid> mapping.
			guid_name_map[instance_guid] = name;
			name_guid_map[type_guid][name] = instance_guid;
		}
	}

	void AssetManagerSystem::Initialize()
	{
		//==Add function loaders for each type==//
		asset_loader_map[TEXTURE] = std::function<bool(const std::filesystem::path&)>(&AssetManagerSystem::LoadTexture);
		asset_loader_map[CUBEMAP] = std::function<bool(const std::filesystem::path&)>(&AssetManagerSystem::LoadCubemap);
		asset_loader_map[MESH] = std::function<bool(const std::filesystem::path&)>(&AssetManagerSystem::LoadMesh);
		asset_loader_map[SHADER] = std::function<bool(const std::filesystem::path&)>(&AssetManagerSystem::LoadShader);
		asset_loader_map[AUDIO] = std::function<bool(const std::filesystem::path&)>(&AssetManagerSystem::LoadAudio);
		asset_loader_map[ARCHTYPE] = std::function<bool(const std::filesystem::path&)>(&AssetManagerSystem::LoadArchtype);

		//Set AAsetManager for android, in audio system.
#ifdef PLATFORM_ANDROID
		AudioSystem::GetInstance().SetAndroidAssetManager(static_cast<void*>(android_asset_manager));
#endif

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
		std::vector<unsigned char> config_json_buffer = GetDataFromFile(std::filesystem::path(ASSET_CONFIG_FILE));
		std::string config_json_string(config_json_buffer.begin(), config_json_buffer.end());
		nlohmann::json config_json = nlohmann::json::parse(config_json_string);

		/*
			Each resource type is located in their own folder.
			Iterate through each asset in that folder.
			For each asset, load using the respective loader (use resource_type to access the loader) and create a full_guid.

			Modify <guid, name> map for serialization.

			//TODO: Multithreading for asset loading here. Ensure no race conditions(like writing to the resource map) and multi-threaded safety within the function loaders too. (e.g. accessing the same file, writing to the same resource map).
		*/
		for (auto& [resource_type, folderpath_string] : config_json.items())
		{
			//Unable to find resource type in map, it might not have been added yet.
			if (asset_loader_map.find(resource_type) == asset_loader_map.end())
			{
				throw std::exception((std::string("No Function Loader for resource type: ") + resource_type + ". Ensure it is spelt correctly, and asset loader has been added to asset_loader_map.").c_str());
			}

			//Call the asset loader for that respective asset type, and pass in the folderpath.
			std::filesystem::path folder_path{ folderpath_string.get<std::string>() };
			if (!asset_loader_map[resource_type](folder_path))
			{
				throw std::exception((std::string("Unable to load asset of type: ") + resource_type).c_str());
			}
		}
	}


	//==Asset Loaders==//
	bool AssetManagerSystem::LoadTexture(const std::filesystem::path& folder_path)
	{
		LOGD("Loading Texture");
		//Acceptable files that can be loaded as a texture.
		std::vector<std::string> texture_extensions{ ".png" };
		std::vector<std::filesystem::path> files = ListFilesInDirectory(folder_path, texture_extensions);

		/*
			Asset names used for mapping to guid.
			Resources are kept in an array until guid mappings are read.
		*/
		std::vector<std::string> asset_names{};
		std::vector< std::shared_ptr<Scene::Texture>> loaded_resources{};

		//Iterate through all texture files
		for (const std::filesystem::path& path : files)
		{
			std::string name = path.stem().string();
			//Allocate resource
			try
			{
				loaded_resources.emplace_back(new Scene::Texture(path.string()));
				asset_names.emplace_back(name); //Done if resource loading is a success.
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
		LOGD("Loading Cubemap");

		/*
			Asset names used for mapping to guid.
			Resources are kept in an array until guid mappings are read.
		*/
		std::vector<std::string> asset_names{};
		std::vector< std::shared_ptr<Scene::Texture>> loaded_resources{};

		//Read the Config File
		std::vector<unsigned char> config_json_buffer = GetDataFromFile(config_file);
		std::string config_json_string(config_json_buffer.begin(), config_json_buffer.end());
		nlohmann::json config_json = nlohmann::json::parse(config_json_string);

		//The config file should contain <name, filepath_arr> pairs
		for (auto& [cubemap_name, filepath_arr] : config_json.items())
		{
			try
			{
				loaded_resources.emplace_back(new Scene::Texture(filepath_arr.get<std::array<std::string, 6>>()));
				asset_names.emplace_back(cubemap_name); //Done if resource loading is a success.
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
		UNREFERENCED_PARAMETER(folder_path);
		LOGI("Mesh Loader not yet implemented.");


		return true;
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
		std::vector<unsigned char> config_json_buffer = GetDataFromFile(config_file);
		std::string config_json_string(config_json_buffer.begin(), config_json_buffer.end());
		nlohmann::json config_json = nlohmann::json::parse(config_json_string);

		//The config file should contain <name, shader_filepaths> pairs
		for (auto& [shader_name, shader_filepaths] : config_json.items())
		{
			//Read vertex and fragment shader filepaths.
			//Always throw an error if not present, as these are critical for a shader program (unlike geometry or tesselation).
			if (!shader_filepaths.contains("vertex"))
			{
				LOGE("%s %s %s", "No vertex filepath in config file for shader program", shader_name.c_str(), ", did you mispell it ?");
				continue;
			}
			if (!shader_filepaths.contains("fragment"))
			{
				LOGE("%s %s %s", "No fragment filepath in config file for shader program", shader_name.c_str(), ", did you mispell it ?");
				continue;
			}
			std::string vertex_shader = shader_filepaths["vertex"];
			std::string fragment_shader = shader_filepaths["fragment"];
			try
			{
				loaded_resources.emplace_back(new ShaderProgram(vertex_shader.c_str(), fragment_shader.c_str()));
				asset_names.emplace_back(shader_name); //Done if resource loading is a success.
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
		std::vector<unsigned char> config_json_buffer = GetDataFromFile(config_file);
		std::string config_json_string(config_json_buffer.begin(), config_json_buffer.end());
		nlohmann::json config_json = nlohmann::json::parse(config_json_string);

		//Reading all filepaths for audio files in the current directory.
		std::filesystem::path folder_path = config_file.parent_path();
		//Acceptable files that can be loaded as an audio asset.
		std::vector<std::string> audio_extensions{ ".mp3" };
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
				loaded_resources.emplace_back(AudioSystem::GetInstance().Load(config["filepath"], config["isLoop"]));
				asset_names.emplace_back(audio_name); //Done if resource loading is a success.
				//if resource was loaded, then remove it from the array so it doesn't get duplicate loaded later on.
				for (auto it = files.begin(); it != files.end(); it++)
				{
					if (std::filesystem::equivalent(*it, std::filesystem::path(config["filepath"].get<std::string>())))
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

	bool AssetManagerSystem::LoadArchtype(const std::filesystem::path& folder_path)
	{
		LOGI("Loading Archtype");
		/*
			Asset names used for mapping to guid.
			Resources are kept in an array until guid mappings are read.
		*/
		std::vector<std::string> asset_names{};
		std::vector< std::shared_ptr<Archtype>> loaded_resources{};
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
				std::vector<unsigned char> temp = GetDataFromFile(path);
				std::string json_string{ temp.begin(), temp.end() };
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

	
} // namespace EngineFramework

