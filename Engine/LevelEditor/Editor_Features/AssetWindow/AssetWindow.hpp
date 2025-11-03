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
		
	};
}

#endif
#endif