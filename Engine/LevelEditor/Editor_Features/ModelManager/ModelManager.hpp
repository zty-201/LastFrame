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

        //==Popup menus for archtypes that were right clicked in the archtype table==//
        

        //==Archtype Modification/Creation==//
        //Creates archtype file, then reloads all archtypes.
        void CreateArchtype(const std::string& name);
        //Creates archtype file of "name" from existing archtype, then reloads all archtypes.
        void CopyArchtype(const std::string& name, const std::shared_ptr<Archtype> existing_archtype);
        //Rename archtype file, modifying guid.meta accordingly.
        void RenameArchtype(const std::string& old_name, const std::string& new_name);
        //Deletes archtype file, modifying guid.meta accordingly. Archtypes are then reloaded.
        void DeleteArchtype(const std::string& name);

        
        //The map of model types selected, allowing the user to pick which model types they want to see from the list.
        std::unordered_map<std::string, bool> selected_types{};
        
        //Name of the archtype selected by drag and drop
        std::string selected_drag_drop_archtype{};
    };
}

#endif
#endif
