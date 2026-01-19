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
#ifndef OBJECTMANAGER_LFEATURE_HPP
#define OBJECTMANAGER_LFEATURE_HPP
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

    class ObjectManager : public LFeature
    {
    public:

        ObjectManager();
        ~ObjectManager() override;

        void Initialize() override;
        void Run() override;
        void Close() override;
        //Handles buttons at the top
        void top_buttons();

        static bool feature_enabled;
    private:
        //Set when user wants to create an archtype from instance, which will open up a popup menu to input additional information.
        //Setting entity data in the tuple will trigger create archtype from instance event.
        entt::entity instance_to_create_archtype = entt::null;

        /*
            Creates an archtype file + asset, reloads archtype assets, then assigns the archtype to the entity.
        */
        void CreateArchtypeFromInstance(entt::entity entity, const std::string& name, const std::string& model_type);
    };
}

#endif
#endif
