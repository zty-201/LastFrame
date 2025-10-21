/*
*****************************************************************/
/*!
\file   FileDialog.cpp
\author Tang Yeun, Dean
\par    y.tang\@digipen.edu
\date   October 2025
\brief
This file is used to facilitate uploading of assets into the asset browser.
Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* *******************************************************************/
#if defined(PLATFORM_WINDOWS) && defined(_DEBUG)
#include "FileDialog.hpp"
#include <windows.h>
#include <commdlg.h>
#include <filesystem>
#include <iostream>

namespace EngineFramework
{
    std::string OpenFileDialog(const char* filter)
    {
        char fileName[MAX_PATH] = { 0 };
        OPENFILENAMEA ofn;
        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = nullptr;
        ofn.lpstrFile = fileName;
        ofn.nMaxFile = sizeof(fileName);
        ofn.lpstrFilter = filter;
        ofn.nFilterIndex = 1;
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

        if (GetOpenFileNameA(&ofn)) {
            return std::string(fileName);
        }
        return {};
    }

    bool CopyFileToProject(const std::string& srcFile, const std::string& destDir)
    {
        try {
            std::filesystem::path source(srcFile);
            std::filesystem::path destination = std::filesystem::path(destDir) / source.filename();

            if (!std::filesystem::exists(destDir)) {
                std::filesystem::create_directories(destDir);
            }

            std::filesystem::copy_file(source, destination,
                std::filesystem::copy_options::overwrite_existing);
            return true;
        }
        catch (const std::filesystem::filesystem_error& e) {
            std::cerr << "Copy failed: " << e.what() << std::endl;
            return false;
        }
    }
}
#endif