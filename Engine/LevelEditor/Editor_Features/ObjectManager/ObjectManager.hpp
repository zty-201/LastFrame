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
#include <vector>
#include <array>
#include <filesystem>
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

        // Filters related helper functions
        static void RemoveEntityFromFilters(entt::entity entity);
        static void SaveFiltersForScene(const std::string& scene_filename);
        static void LoadFiltersForScene(const std::string& scene_filename);
        static bool feature_enabled;
    private:
        struct CustomFilter
        {
            std::string name;
            std::vector<entt::entity> members;
            bool expanded = true;
        };

        //Set when user wants to create an archtype from instance, which will open up a popup menu to input additional information.
        //Setting entity data in the tuple will trigger create archtype from instance event.
        entt::entity instance_to_create_archtype = entt::null;
        std::vector<CustomFilter> m_CustomFilters;
        std::array<char, 64> m_NewFilterNameBuffer{};

        /*
            Creates an archtype file + asset, reloads archtype assets, then assigns the archtype to the entity.
        */
        void CreateArchtypeFromInstance(entt::entity entity, const std::string& name, const std::string& model_type);
        void DrawCustomFilters(iEditorView& selection_state,
            bool has_search_query,
            const std::string& search_query_lower,
            bool numeric_search,
            uint32_t search_id);
        entt::entity DuplicateAndSelect(entt::entity source_entity, iEditorView& selection_state);
        bool DeleteEntity(entt::entity target_entity, iEditorView& selection_state);
        void BeginRename(entt::entity entity);

        // Filters related stuff
        void CleanupFilters();
        void RemoveEntityFromAllFilters(entt::entity entity);
        std::string SerializeFiltersToJson() const;
        void DeserializeFiltersFromJson(const std::string& json_text);
        static std::filesystem::path GetFilterSavePath(const std::string& scene_filename);
        static bool AddEntityToFilter(CustomFilter& filter, entt::entity entity);
        static void RemoveEntityFromFilter(CustomFilter& filter, entt::entity entity);
        static std::string TrimString(const std::string& value);
        bool IsEntityInFilters(entt::entity entity) const;

        static inline ObjectManager* s_Instance{};
        bool m_OpenArchtypePopupOnce = false;
        bool m_OpenRenamePopup = false;
        entt::entity m_RenameTarget = entt::null;
        std::array<char, 128> m_RenameBuffer{};
    };
}

#endif
#endif
