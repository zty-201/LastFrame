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
/*
*****************************************************************/
/*!
\file   ModelManager.hpp
\author Tan Yee Tong
\par    yeetong.t\@digipen.edu
\date   September 2025
\brief
This file shows the models currently loaded in the scene of the level editor
Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* *******************************************************************/

#ifndef MODELMANAGER_LFEATURE_HPP
#define MODELMANAGER_LFEATURE_HPP
#include <unordered_map>
#include <string>
#include <sstream>
#include "ISystem.hpp"
#include "entt/entity/registry.hpp"
#include "System_LevelEditor.hpp"
#include "LFeature.hpp"
#include "Archtype.hpp"


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
    private:
        //==Represents the top, left and right pane.
        void DrawSelectedTypes();
        void DrawTypeList(const std::unordered_map<std::string, std::vector<int>>& archtype_type_map);
        void DrawSelectedArchtypeTable(int spacing, int per_row, std::unordered_map<std::string, std::vector<int>>& archtype_type_map, const std::vector<std::pair<xresource::full_guid, std::shared_ptr<Archtype>>>& archtype_arr);

        //==Popup menus for archtypes that were right clicked in the archtype table. 
        /*    
        * TODO:
            Popup Menu (Right click archtype button):
		    - Enable dragdrop into scene (create instance with default transform at where camera is pointing, with option to choose normalized or world pos).
            --> First create instance
            --> Next do registry.emplace_or_replace.
		    --> Do this in a function overload for GameObjectSystem::CreateInstance().
            - Creation of instance
		    - Copy of existing archtype
		    - Rename of archtype
		    - Switch archtype model_type
            - Deletion of archtype

            Others
            - Creation of new archtype
            
            When renaming or deleting an archtype, ensure 
            - guid.meta file is changed
            - guid_to_name map is changed.
        */
        

        //The map of model types selected, allowing the user to pick which model types they want to see from the list.
        std::unordered_map<std::string, bool> selected_types{};
    };
}

#endif
#endif
