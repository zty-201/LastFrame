/*
*****************************************************************/
/*!
\file   ObjectManager.hpp
\author Tang Yeun, Dean
\par    y.tang\@digipen.edu
\date   September 2025
\brief
This file runs the Scene Hierarchy of the level editor
Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* *******************************************************************/
#if defined(PLATFORM_WINDOWS) && defined(_DEBUG)
#ifndef MODELMANAGER_LFEATURE_HPP
#define MODELMANAGER_LFEATURE_HPP
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

    class ModelManager : public LFeature
    {
    public:

        ModelManager();
        ~ModelManager() override;

        void Initialize() override;
        void Run() override;
        void Close() override;

        static bool feature_enabled;
    };
}

#endif
#endif
