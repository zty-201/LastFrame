/*
*****************************************************************/
/*!
\file   FileDialog.hpp
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
#pragma once
#include <string>

namespace EngineFramework
{
    // Opens file explorer and lets the user select a file
    // Returns the selected file path, or empty string if canceled
    std::string OpenFileDialog(const char* filter = "All Files\0*.*\0");

    // Copies a selected file into a destination directory (overwrites if exists)
    bool CopyFileToProject(const std::string& srcFile, const std::string& destDir);
}
#endif