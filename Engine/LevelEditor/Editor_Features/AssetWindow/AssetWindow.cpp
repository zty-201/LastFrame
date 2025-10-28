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
#if defined(PLATFORM_WINDOWS) && defined(_DEBUG)

#include "pch.hpp"
#include "AssetWindow.hpp"
#include "Assets/System/AssetManager.hpp"
#include "FileDialog.hpp"
//#include <stb_image.h>
//#include <glad/glad.h>

namespace EngineFramework
{
	AssetWindow::AssetWindow()
	{
		rootAssetDirectory = currentDirectory = AssetManagerSystem::GetInstance().GetAssetFolderPrefix().string() + "Extern"; // Start at project asset root
	}

	AssetWindow::~AssetWindow()
	{
        //UnloadTextures();
	}

	void AssetWindow::Initialize()
	{
        /*m_FolderIcon = LoadTextureFromFile("")*/
	}

	void AssetWindow::Run()
	{
        if (!is_enabled) return;

        if (ImGui::Begin("Asset Browser", &is_enabled))
        {
            // --- Header --- 
            ImGui::Text("Current: %s", currentDirectory.filename().string().c_str());
            ImGui::Separator();
            if (currentDirectory != rootAssetDirectory && currentDirectory.has_parent_path()) {
                if (ImGui::Button("Back")) {
                    currentDirectory = currentDirectory.parent_path();
                }
            }

            // Place Upload button on the far right
            float windowWidth = ImGui::GetContentRegionAvail().x;
            float buttonWidth = 100.0f;  // whatever size you want
            ImGui::SameLine(windowWidth - buttonWidth);

            if (ImGui::Button("Upload", ImVec2(buttonWidth, 0))) {
                std::string file = EngineFramework::OpenFileDialog("All Files\0*.*\0");
                if (!file.empty()) {
                    EngineFramework::CopyFileToProject(file, currentDirectory.string());
                }
            }

            if (ImGui::Button("Refresh", ImVec2(buttonWidth, 0))) {
                AssetManagerSystem::GetInstance().RemoveAllResources();
                AssetManagerSystem::GetInstance().LoadAllResources();
            }

            ImGui::Separator();
            

            // --- Layout ---
            static float padding = 5.0f;
            static float thumbnailSize = 72.0f;
            float cellSize = thumbnailSize + padding;

            float panelWidth = ImGui::GetContentRegionAvail().x;
            int columns = static_cast<int>(panelWidth / cellSize);
            if (columns < 1) columns = 1;

            ImGui::Columns(columns, 0, false);

            bool directoryChanged = false;
            std::filesystem::path newDirectory;

            // --- Draw folders ---
            for (auto& entry : std::filesystem::directory_iterator(currentDirectory))
            {
                if (entry.is_directory())
                {
                    std::string folderName = entry.path().filename().string();
                    ImGui::PushID(folderName.c_str());

                    if (ImGui::Button("Folder", { thumbnailSize, thumbnailSize })) {
                        directoryChanged = true;
                        newDirectory = entry.path();
                    }
                    ImGui::TextWrapped("%s", folderName.c_str());

                    ImGui::NextColumn();
                    ImGui::PopID();
                }
            }

            // --- Draw files ---
            for (auto& entry : std::filesystem::directory_iterator(currentDirectory))
            {
                if (!entry.is_directory())
                {
                    std::string fileName = entry.path().filename().string();
                    ImGui::PushID(fileName.c_str());

                    std::string extension = entry.path().extension().string();

                    // Draw simple button
                    if (ImGui::Button(extension.c_str(), {thumbnailSize, thumbnailSize})) {
                        selectedAsset = entry.path();
                    }

                    // Begin drag-drop source for all file types
                    if (ImGui::BeginDragDropSource())
                    {
                        // Payload type
                        std::string pathStr = entry.path().string();

                        //*****Set payload type for targets with specific requirements*****
                        const char* payloadType;
                        if (extension == ".png") payloadType = "Textures";
                        //else if (extension == ".xyz" || extension == ".abc") payloadType = "defghi";
                        else payloadType = "GENERAL";

                        ImGui::SetDragDropPayload(payloadType, pathStr.c_str(), pathStr.size() + 1);

                        // Preview text + placeholder thumbnail
                        ImGui::TextUnformatted(fileName.c_str());
                        ImGui::Image((ImTextureID)0, ImVec2(50, 50), ImVec2(0, 1), ImVec2(1, 0));
                        ImGui::EndDragDropSource();
                    }
                    

                    ImGui::TextWrapped("%s", fileName.c_str());

                    ImGui::NextColumn();
                    ImGui::PopID();
                }
            }

            if (directoryChanged) {
                currentDirectory = newDirectory;
            }

            ImGui::Columns(1);

            // --- Controls ---
            ImGui::SliderFloat("Thumbnail Size", &thumbnailSize, 32.0f, 256.0f);
            ImGui::SliderFloat("Padding Size", &padding, 0.0f, 24.0f);
        }
        ImGui::End();
	}

	void AssetWindow::Close()
	{
	}
}
#endif
