/*
*****************************************************************/
/*!
\file   ObjectManager.hpp
\author Tan Yee Tong
\par    yeetong.t\@digipen.edu
\date   October 2025
\brief
This file runs the Scene Hierarchy of the level editor
Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* *******************************************************************/
#if defined(PLATFORM_WINDOWS) && defined(_DEBUG)
#ifndef GAMEPLAYKEYMANAGER_LFEATURE_HPP
#define GAMEPLAYKEYMANAGER_LFEATURE_HPP
#include <unordered_map>
#include <string>
#include <sstream>
#include "ISystem.hpp"
#include "entt/entity/registry.hpp"
#include "System_LevelEditor.hpp"
#include "LFeature.hpp"


namespace EngineFramework
{
    struct iEditorView; //forward declaration

    class Gameplay_KeyManager : public LFeature
    {
    public:

        Gameplay_KeyManager();
        ~Gameplay_KeyManager() override;

        void Initialize() override;
        void Run() override;
        void Close() override;

        static bool feature_enabled;
    private:
        void add_key(std::string, ImGuiKey);
        void remove_key(std::string);
        bool valid_string(std::string);
    };
}

#endif
#endif
