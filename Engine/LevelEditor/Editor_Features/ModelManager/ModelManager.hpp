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
    
        enum ModelErrorCode
        {
            NOT_FOUND,
            FILE_ALREADY_EXISTS,
            CANNOT_MODIFY_GUIDMETA,
            INVALID_ARGUMENTS,
            SUCCESS,
            UNKNOWN_FAILURE
        };
    private:
        //Used to store archtype commands by the user, so that it can be run at the end of the Run() cycle.
        struct ArchtypeEvent
        {
            enum Event
            {
                CREATE,
                COPY,
                RENAME,
                DELETE,
                NONE
            };
            ArchtypeEvent() { event = Event::NONE; }
            ArchtypeEvent(Event event, std::string current_name = {}, std::string new_name = {}, std::shared_ptr<Archtype> archtype = nullptr)
            : event{event}, current_name{current_name}, new_name{new_name}, archtype{archtype}
            {
            }
            Event event;
            std::string current_name{};
            std::string new_name{};
            std::shared_ptr<Archtype> archtype{};
        };
        //==Represents the top, left and right pane.
        void DrawSelectedTypes();
        void DrawTypeList(const std::unordered_map<std::string, std::vector<int>>& archtype_type_map);
        void DrawSelectedArchtypeTable(int spacing, int per_row, std::unordered_map<std::string, std::vector<int>>& archtype_type_map, const std::vector<std::pair<xresource::full_guid, std::shared_ptr<Archtype>>>& archtype_arr);

        //==Popup menus for archtypes that were right clicked in the archtype table==//
        void ArchtypePopupMenu(std::string archtype_name, const std::shared_ptr<Archtype> archtype);

        //==Archtype Modification/Creation==//
        //Note: Ensure if any of these functions are called, that they are done at the end of Run().
        //This prevents invalidation of any map that currently uses std::shared_ptr<Archtype>
        //Creates archtype file, then reloads all archtypes. 
        ModelErrorCode CreateArchtype(const std::string& name);
        //Creates archtype file of "name" from existing archtype, then reloads all archtypes. 
        ModelErrorCode CopyArchtype(const std::string& name, const std::shared_ptr<Archtype> existing_archtype);
        //Rename archtype file, modifying guid.meta accordingly. 
        ModelErrorCode RenameArchtype(const std::string& old_name, const std::string& new_name);
        //Deletes archtype file, modifying guid.meta accordingly. Archtypes are then reloaded. 
        ModelErrorCode DeleteArchtype(const std::string& name);
        
        
        //The map of model types selected, allowing the user to pick which model types they want to see from the list.
        std::unordered_map<std::string, bool> selected_types{};
        
        //Name of the archtype selected by drag and drop
        std::string selected_drag_drop_archtype{};

        //Due to reload/renaming of archtypes, any shared_ptr<Archtype> map or indices can be affected by archtype modification/creation. Thus, only do these commands at the end of the run() cycle.
        ArchtypeEvent delayed_archtype_event;
    };
}

#endif
#endif
