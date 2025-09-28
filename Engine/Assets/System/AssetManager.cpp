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

//Asset structs
#include "scene.h"
#include "ShaderProgram.h"
#include "Audio.hpp"
#include "SoundAsset.hpp"
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

	void AssetManagerSystem::Initialize()
	{
		//==Add function loaders for each type==//
		asset_loader_map[TEXTURE] = std::function<bool(const std::filesystem::path&)>(&AssetManagerSystem::LoadTexture);
		asset_loader_map[CUBEMAP] = std::function<bool(const std::filesystem::path&)>(&AssetManagerSystem::LoadCubemap);
		asset_loader_map[MESH] = std::function<bool(const std::filesystem::path&)>(&AssetManagerSystem::LoadMesh);
		asset_loader_map[SHADER] = std::function<bool(const std::filesystem::path&)>(&AssetManagerSystem::LoadShader);
		asset_loader_map[AUDIO] = std::function<bool(const std::filesystem::path&)>(&AssetManagerSystem::LoadAudio);

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
		//Used as part of the final assets' guid.
		xresource::type_guid type_guid{ TEXTURE };
		//Acceptable files that can be loaded as a texture.
		std::vector<std::string> texture_extensions{ ".png" };

		std::vector<std::filesystem::path> files = ListFilesInDirectory(folder_path, texture_extensions);

		//Iterate through all texture files
		for (const std::filesystem::path& path : files)
		{
			//Get guid (and update guid_name_map by calling GetFullGuid.
			std::string name = path.stem().string();

			//Allocate resource
			try
			{
				std::shared_ptr<Scene::Texture> resource(new Scene::Texture(path.string()));
				xresource::full_guid asset_guid = GetFullGuid(type_guid, name); //Only if resource is successfully loaded.
				AddResource(asset_guid, resource);
			}
			catch (std::exception& e)
			{
				LOGE("%s %s %s", "Unable to Load Texture", name.c_str(), e.what());
			}
		}

		return true;
	}
	bool AssetManagerSystem::LoadCubemap(const std::filesystem::path& config_file)
	{
		LOGD("Loading Cubemap");
		//Used as part of the final asset guid.
		xresource::type_guid type_guid{ CUBEMAP };
		//Read the Config File
		std::vector<unsigned char> config_json_buffer = GetDataFromFile(config_file);
		std::string config_json_string(config_json_buffer.begin(), config_json_buffer.end());
		nlohmann::json config_json = nlohmann::json::parse(config_json_string);

		//The config file should contain <name, filepath_arr> pairs
		for (auto& [cubemap_name, filepath_arr] : config_json.items())
		{
			try
			{
				//Allocate resource
				std::shared_ptr<Scene::Texture> resource(new Scene::Texture(filepath_arr.get<std::array<std::string, 6>>()));
				xresource::full_guid asset_guid = GetFullGuid(type_guid, cubemap_name); //Only if resource is successfully loaded.
				AddResource(asset_guid, resource);
			}
			catch (std::exception& e)
			{
				LOGE("%s %s %s", "Unable to Load Cubemap", cubemap_name.c_str(), e.what());
			}
		}
		return true;
	}
	bool AssetManagerSystem::LoadMesh(const std::filesystem::path& folder_path)
	{
		UNREFERENCED_PARAMETER(folder_path);
		xresource::type_guid type_guid{ MESH };
		LOGI("Mesh Loader not yet implemented.");
		return true;
	}
	bool AssetManagerSystem::LoadShader(const std::filesystem::path& config_file)
	{
		xresource::type_guid type_guid{ SHADER };
		LOGI("Loading Shader");

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
				//Allocate resource
				std::shared_ptr<ShaderProgram> resource(new ShaderProgram(vertex_shader.c_str(), fragment_shader.c_str()));
				xresource::full_guid asset_guid = GetFullGuid(type_guid, shader_name);
				AddResource(asset_guid, resource);
			}
			catch (std::exception& e)
			{
				LOGE("%s %s %s", "Unable to Load Shader", shader_name.c_str(), e.what());
			}
		}
		return true;
	}
	bool AssetManagerSystem::LoadAudio(const std::filesystem::path& config_file)
	{
		xresource::type_guid type_guid{ AUDIO };
		LOGI("Loading Audio");

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
				//Allocate resource
				std::shared_ptr<SoundAsset> resource = AudioSystem::GetInstance().Load(config["filepath"], config["isLoop"]);
				xresource::full_guid asset_guid = GetFullGuid(type_guid, audio_name);
				AddResource(asset_guid, resource);
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
				//Allocate resource
				std::shared_ptr<SoundAsset> resource = AudioSystem::GetInstance().Load(audio_path.string(), false);
				xresource::full_guid asset_guid = GetFullGuid(type_guid, audio_name);
				AddResource(asset_guid, resource);
			}
			catch (std::exception& e)
			{
				LOGE("%s %s %s", "Unable to Load Audio File", audio_name.c_str(), e.what());
			}
		}

		return true;
	}

	//// -------- small helpers --------
	///**
	// * @brief Safely canonicalize a path (ignores errors).
	// * @param p Input path.
	// * @return Canonical/normalized path; may equal @p p if normalization failed.
	// */
	//static std::filesystem::path CanonicalSafe(const std::filesystem::path& p) {
	//	std::error_code ec;
	//	return std::filesystem::weakly_canonical(p, ec);
	//}

	///**
	// * @brief Check if path @p a starts with @p prefix (element-wise).
	// * @details Used to ensure a destination stays inside the root directory.
	// */
	//static bool StartsWithPath(const std::filesystem::path& a, const std::filesystem::path& prefix) {
	//	auto ca = CanonicalSafe(a);
	//	auto cp = CanonicalSafe(prefix);
	//	auto ait = ca.begin(), aend = ca.end();
	//	auto pit = cp.begin(), pend = cp.end();
	//	for (; pit != pend && ait != aend; ++pit, ++ait) if (*pit != *ait) return false;
	//	return pit == pend; // every element of prefix matched
	//}

	///**
	// * @brief Lowercase utility for extensions and case-insensitive compares.
	// */
	//std::string AssetManagerSystem::toLower(std::string s) {
	//	for (auto& c : s) c = static_cast<char>(::tolower(static_cast<unsigned char>(c)));
	//	return s;
	//}



	//void AssetManagerSystem::SetRoot(const std::filesystem::path& root) {
	//	m_root = root;
	//	std::error_code ec;
	//	std::filesystem::create_directories(m_root, ec); // best-effort
	//}

	//std::filesystem::path AssetManagerSystem::full(const std::filesystem::path& rel) const {
	//	return rel.is_absolute() ? rel : (m_root / rel).lexically_normal();
	//}
	//bool AssetManagerSystem::insideRoot(const std::filesystem::path& abs) const {
	//	return StartsWithPath(abs, m_root);
	//}

	//// -------- registration --------
	//AssetID AssetManagerSystem::Register(const std::string& name,
	//	const std::filesystem::path& relativePath,
	//	bool verifyExists)
	//{
	//	Entry e;
	//	e.id = m_next++;
	//	e.name = name;
	//	e.relative = relativePath.lexically_normal();

	//	if (verifyExists) {
	//		std::error_code ec;
	//		if (!std::filesystem::exists(full(e.relative), ec)) {
	//			// still register, but mark lastWrite default (not required)
	//		}
	//		else {
	//			e.lastWrite = std::filesystem::last_write_time(full(e.relative), ec);
	//		}
	//	}

	//	m_byName[name] = e;
	//	m_byId[e.id] = name;

	//	return e.id;
	//}

	//void AssetManagerSystem::Unregister(const std::string& name) {
	//	auto it = m_byName.find(name);
	//	if (it == m_byName.end()) return;
	//	AssetID id = it->second.id;
	//	m_byName.erase(it);
	//	m_byId.erase(id);
	//}

	//bool AssetManagerSystem::Has(const std::string& name) const {
	//	return m_byName.find(name) != m_byName.end();
	//}

	//AssetID AssetManagerSystem::FindId(const std::string& name) const {
	//	auto it = m_byName.find(name);
	//	return (it == m_byName.end()) ? 0 : it->second.id;
	//}

	//// -------- queries --------
	//std::filesystem::path AssetManagerSystem::GetRelativePath(AssetID id) const {
	//	auto idIt = m_byId.find(id);
	//	if (idIt == m_byId.end()) return {};

	//	auto nmIt = m_byName.find(idIt->second);
	//	if (nmIt == m_byName.end()) return{};

	//	return nmIt->second.relative;
	//}
	//std::filesystem::path AssetManagerSystem::GetAbsolutePath(AssetID id) const {
	//	auto idIt = m_byId.find(id);
	//	if (idIt == m_byId.end()) return{};

	//	auto nmIt = m_byName.find(idIt->second);
	//	if (nmIt == m_byName.end()) return{};

	//	return full(nmIt->second.relative);
	//}
	//bool AssetManagerSystem::ExistsOnDisk(AssetID id) const {
	//	auto idIt = m_byId.find(id);
	//	if (idIt == m_byId.end()) return false;

	//	auto nmIt = m_byName.find(idIt->second);
	//	if (nmIt == m_byName.end()) return false;

	//	std::error_code ec;
	//	return std::filesystem::exists(full(nmIt->second.relative), ec);
	//}
	//bool AssetManagerSystem::ExistsOnDiskByName(const std::string& name) const {
	//	auto it = m_byName.find(name);
	//	if (it == m_byName.end()) return false;

	//	std::error_code ec;
	//	return std::filesystem::exists(full(it->second.relative), ec);
	//}

	//// -------- relocation / rename --------
	//bool AssetManagerSystem::RelocateByName(const std::string& name,
	//	const std::filesystem::path& newRelativePath,
	//	MovePolicy policy,
	//	bool allowOverwrite)
	//{
	//	auto it = m_byName.find(name);
	//	if (it == m_byName.end()) return false;

	//	Entry& e = it->second;

	//	const auto oldAbs = full(e.relative);
	//	const auto newRel = newRelativePath.lexically_normal();
	//	const auto newAbs = full(newRel);

	//	// keep everything inside root
	//	if (!insideRoot(newAbs)) return false;

	//	std::error_code ec;

	//	if (policy != MovePolicy::UpdateRecordOnly) {
	//		// make sure destination dir exists
	//		std::filesystem::create_directories(newAbs.parent_path(), ec);

	//		const bool destExists = std::filesystem::exists(newAbs, ec);
	//		if (destExists && !allowOverwrite) return false;

	//		if (policy == MovePolicy::MoveOnDisk) {
	//			// try fast rename; if cross-device, fall back to copy+remove
	//			std::filesystem::rename(oldAbs, newAbs, ec);
	//			if (ec) {
	//				ec.clear();
	//				if (!std::filesystem::copy_file(
	//					oldAbs, newAbs,
	//					allowOverwrite ? std::filesystem::copy_options::overwrite_existing
	//					: std::filesystem::copy_options::none,
	//					ec))
	//					return false;
	//				std::filesystem::remove(oldAbs, ec);
	//			}
	//		}
	//		else { // CopyOnDisk
	//			if (!std::filesystem::copy_file(
	//				oldAbs, newAbs,
	//				allowOverwrite ? std::filesystem::copy_options::overwrite_existing
	//				: std::filesystem::copy_options::none,
	//				ec))
	//				return false;
	//		}
	//	}

	//	// update the record
	//	e.relative = newRel;
	//	e.lastWrite = std::filesystem::last_write_time(newAbs, ec);
	//	return true;
	//}


	//bool AssetManagerSystem::RenameName(const std::string& oldName, const std::string& newName) {
	//	if (m_byName.find(newName) != m_byName.end()) return false; // avoid collision
	//	auto it = m_byName.find(oldName);
	//	if (it == m_byName.end()) return false;

	//	Entry e = it->second;
	//	m_byName.erase(it);
	//	e.name = newName;
	//	m_byName[newName] = e;
	//	m_byId[e.id] = newName;
	//	return true;
	//}

	//// -------- discovery --------
	//std::size_t AssetManagerSystem::ImportFolder(const std::filesystem::path& relativeDir,
	//	const std::vector<std::string>& extensions,
	//	bool recursive)
	//{
	//	std::size_t count = 0;
	//	std::vector<std::string> extsLower;
	//	extsLower.reserve(extensions.size());
	//	for (auto s : extensions) extsLower.push_back(toLower(std::move(s)));

	//	auto mode = recursive ? std::filesystem::directory_options::follow_directory_symlink
	//		: std::filesystem::directory_options::none;

	//	std::error_code ec;
	//	const auto baseAbs = full(relativeDir);
	//	if (!std::filesystem::exists(baseAbs, ec)) return 0;

	//	auto matchExt = [&](const std::filesystem::path& p) {
	//		auto ext = toLower(p.extension().string());
	//		for (auto& e : extsLower) if (ext == e) return true;
	//		return extsLower.empty(); // if you pass {}, accept all
	//		};

	//	if (recursive) {
	//		for (auto& de : std::filesystem::recursive_directory_iterator(baseAbs, mode, ec)) {
	//			if (!de.is_regular_file(ec)) continue;
	//			const auto abs = de.path();
	//			if (!matchExt(abs)) continue;
	//			auto rel = std::filesystem::relative(abs, m_root, ec).lexically_normal();
	//			auto name = abs.stem().string(); // basic name; collisions possible
	//			if (!Has(name)) { Register(name, rel, /*verify=*/false); ++count; }
	//		}
	//	}
	//	else {
	//		for (auto& de : std::filesystem::directory_iterator(baseAbs, ec)) {
	//			if (!de.is_regular_file(ec)) continue;
	//			const auto abs = de.path();
	//			if (!matchExt(abs)) continue;
	//			auto rel = std::filesystem::relative(abs, m_root, ec).lexically_normal();
	//			auto name = abs.stem().string();
	//			if (!Has(name)) { Register(name, rel, /*verify=*/false); ++count; }
	//		}
	//	}
	//	return count;
	//}

	//std::vector<std::pair<std::string, std::filesystem::path>>
	//	AssetManagerSystem::List() const {
	//	std::vector<std::pair<std::string, std::filesystem::path>> out;
	//	out.reserve(m_byName.size());
	//	for (auto& [n, e] : m_byName) out.emplace_back(n, e.relative);
	//	return out;
	//}

	//void AssetManagerSystem::SetAudioSystem(AudioSystem* audio)
	//{
	//	m_audioSystem = audio;
	//}

	//bool AssetManagerSystem::LoadAudio(const std::string& id,
	//	const std::string& nameOrRel,
	//	bool loop)
	//{
	//	if (!m_audioSystem)
	//	{
	//		LOGI("%s %s", "[AssetManager] No AudioSystem set, unable to load", id.c_str());
	//		return false;
	//	}

	//	// 1) Resolve a path
	//	std::string resolved;

	//	// CaseA: treat nameOrRel as a registered logical name
	//	if (Has(nameOrRel))
	//	{
	//		AssetID aid = FindId(nameOrRel);
	//		const std::filesystem::path abs = GetAbsolutePath(aid);
	//		resolved = abs.string();
	//	}

	//	else
	//	{
	//		// CaseB: treat as a filename under Extern
	//		const std::filesystem::path rel = std::filesystem::path(m_audioSubdir) / nameOrRel;
	//		const std::filesystem::path abs = full(rel);
	//		resolved = abs.string();
	//	}

	//	// 2) Ask AudioSystem to load
	//	const bool ok = m_audioSystem->Load(id, resolved, loop);
	//	if (ok)
	//	{
	//		LOGI("%s %s %s %s", "[AssetManager] Loaded Audio ", id.c_str(), " from ", resolved.c_str());
	//	}
	//	else
	//	{
	//		LOGI("%s %s %s %s", "[AssetManager] FAILED to load audio ", id.c_str(), " from ", resolved.c_str());
	//	}

	//	return ok;
	//}

} // namespace EngineFramework

