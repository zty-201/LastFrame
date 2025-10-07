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
//#include <imgui.h>

//#ifndef IMGUI_API
//typedef void* ImTextureID;
//#endif

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

	private:
		// Recursive draw is replaced with a grid-style explorer
		std::filesystem::path currentDirectory;
		std::filesystem::path selectedAsset;

		//Thumbnail and icon
		/*std::unordered_map<std::string, ImTextureID> m_Thumbnails;
		ImTextureID m_FolderIcon = NULL;
		ImTextureID m_FileIcon = NULL;

		ImTextureID LoadTextureFromFile(const std::string& path);
		void UnloadTextures();*/
	};
}

#endif
#endif