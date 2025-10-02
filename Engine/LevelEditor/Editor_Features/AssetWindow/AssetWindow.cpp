/*
*****************************************************************/
/*!
\file   AssetWindow.cpp
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
#include "pch.hpp"
#include "AssetWindow.hpp"
#include "Assets/System/AssetManager.hpp"
#include <filesystem>

namespace EngineFramework
{
	AssetWindow::AssetWindow()
	{
	}

	AssetWindow::~AssetWindow()
	{
	}

	void AssetWindow::Initialize()
	{
	}

	void AssetWindow::Run()
	{
		if (!is_enabled) return;

		if (ImGui::Begin("Asset Browser", &is_enabled))
		{
			ImGui::Text("Project Assets");
			ImGui::Separator();

			DrawDirectoryRecursive("Extern");
		}
		ImGui::End();
	}

	void AssetWindow::Close()
	{
	}

	void AssetWindow::DrawDirectoryRecursive(const std::filesystem::path& dir)
	{
		// Only draw valid directories
		if (!std::filesystem::exists(dir) || !std::filesystem::is_directory(dir))
			return;

		// Show this folder as a tree node
		if (ImGui::TreeNode(dir.filename().string().c_str())) {
			for (auto& entry : std::filesystem::directory_iterator(dir)) {
				if (entry.is_directory()) {
					DrawDirectoryRecursive(entry.path());
				}
				else {
					// Draw a file
					ImGui::Selectable(entry.path().filename().string().c_str());
				}
			}
			ImGui::TreePop();
		}
	}
}

