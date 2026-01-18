/*
*****************************************************************/
/*!
\file   AssetWindow.hpp
\author Tang Yeun, Dean
\par    y.tang\@digipen.edu
\date   September 2025
\brief
This file allows the user to find all game assets
Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* *******************************************************************/
#if defined(PLATFORM_WINDOWS) && defined(_DEBUG)
#ifndef ASSETWINDOW_LFEATURE_HPP
#define ASSETWINDOW_LFEATURE_HPP

#include "ISystem.hpp"
#include "entt/entity/registry.hpp"
#include "System_LevelEditor.hpp"
#include "LFeature/LFeature.hpp"
#include <filesystem>
#include <set>
#include <unordered_map>

namespace EngineFramework
{
	struct iEditorview; //Forward declaration

	class AssetWindow : public LFeature
	{
	public:
		AssetWindow();
		~AssetWindow() override;

		void Initialize() override;
		void Run() override;
		void Close() override;

		static bool feature_enabled;
		
		static AssetWindow& GetInstance() { return *this_class; }
		static inline AssetWindow* this_class = nullptr;

		void seralise();
		bool get_tags(const std::string& component, const std::string& variable, std::vector<std::string>& tags);
		void sample_drag_drop();
	private:
		// Recursive draw is replaced with a grid-style explorer
		std::filesystem::path rootAssetDirectory;
		std::filesystem::path currentDirectory;
		std::filesystem::path selectedAsset;
		std::map <std::string, std::map <std::string,std::set<std::string>>> component_mapping;
		std::map <std::string, std::map <std::string, bool>> component_validity_flags;
		std::vector<std::string> valid_extensions;

		bool is_window;
		void open_window();
		void init_flags();
		void init_extensions();
		
		void deseralise();
		const bool& flag(const std::string&, const std::string&) const; //flag_state read only
		bool& flag(const std::string&, const std::string&); //flag_state

		void update_map();
		bool check_extension_validity(std::string);

		struct ThumbnailEntry
		{
			unsigned int textureId = 0;
			std::filesystem::file_time_type lastWrite{};
			bool failed = false;
		};

		// Thumbnail relevant stuff
		// A cache to store the thumbnails for the textures (jpg, png etc)
		std::unordered_map<std::string, ThumbnailEntry> thumbnail_cache;

		// To get the thumbnail texture as it is the cache entry point
		// Given a file path it will query the file last write time and look up to the existing entry within the cache
		// If the cache entry is valid, it will return the stored GL texture id, otherwise it deletes the old GL texture, remove entry and calls for LoadThumbnail to rebuild a new one
		// Then store the new entry back in to the cache before returning the texture id
		unsigned int GetThumbnailTexture(const std::filesystem::path& path);

		// Use stbi_load to decode large image into rgba pixels and create a GL texture, set linear flitering + clamp wrap and uploads these pixels via glTexImage2D
		// Then restore the unpacked alignment and frees up the cpu buffer
		// Next, fill up the thumbnail entry with the generated texture id and report success or failure
		bool LoadThumbnailTexture(const std::filesystem::path& path, ThumbnailEntry& outEntry);

		// Destroying the assetwindow or refreshing it will clear the cache
		void ClearThumbnailCache();
		
	};
}

#endif
#endif
