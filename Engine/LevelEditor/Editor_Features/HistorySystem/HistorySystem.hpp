/*
*****************************************************************/
/*!
\file   HistorySystem.hpp
\author Tan Yee Tong
\par    yeetong.t\@digipen.edu
\date   September 2025
\brief
This file runs the Entity based history system of the level editor
Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* *******************************************************************/
#if defined(PLATFORM_WINDOWS) && defined(_DEBUG)

#ifndef HISTORYSYSTEM_LFEATURE_HPP
#define HISTORYSYSTEM_LFEATURE_HPP
#include <unordered_map>
#include <string>
#include <sstream>
#include "ISystem.hpp"
#include "entt/entity/registry.hpp"
#include "System_LevelEditor.hpp"
#include <deque>

namespace EngineFramework
{
    
    /*       
        case 1: component is removed
        case 2: component is added
        case 3: editing of component variables
        case 4: entity is deleted
        case 5: a new entity is added
    */
    enum Record_Type {
        COMP_ADDED, COMP_REMOVED, COMP_EDITED, ENTITY_ADDED, ENTITY_REMOVED, INVALID
    };

    class Record
    {

    public:
        virtual ~Record() = default;
        Record_Type Type;
        entt::entity entity;
        virtual void pop() = 0;
    };

    /*
    * Case 1 and 2
    */

    class Record_Component_Whole : public Record
    {
    public:
        std::string component;
        std::map<std::string, entt::meta_any> variables;
        virtual void pop() override;
        virtual ~Record_Component_Whole() override;
    };

    /*
    * Case 3
    */

    class Record_Component_Single : public Record
    {
    public:
        std::string component;
        std::pair<std::string, entt::meta_any> variable;
        virtual void pop() override;
        virtual ~Record_Component_Single() override;
    };

    /*
    * Case 4 and 5
    */
    class Record_Entity : public Record
    {
    public:
        std::map<std::string, std::map<std::string, entt::meta_any>> components_variables;
        virtual void pop() override;
        virtual ~Record_Entity() override;
    };


    class HistorySystem
    {
    public:

        bool is_meta_component(const entt::meta_type& meta) {
            if (!meta || !meta.is_class()) {
                return false;
            }
            using namespace entt::literals;
            return meta.func("emplace"_hs) && meta.func("get_ref"_hs);
        }
        static HistorySystem& GetInstance() { return *this_class; }
        static inline HistorySystem* this_class = nullptr;

        HistorySystem(size_t value);
        ~HistorySystem();
        void Initialize();
       
        std::unique_ptr<Record> start_track(entt::entity e, std::string component, Record_Type type);//case 1 and 2 
        std::unique_ptr<Record> start_track(entt::entity e, std::string component, std::string var_name, Record_Type type = Record_Type::COMP_EDITED); //case 3
        std::unique_ptr<Record> start_track(entt::entity e, Record_Type type); //case 4 and 5
        
        void end_track(std::unique_ptr<Record>& rec, const entt::meta_any& new_value); //case 3
        void end_track(std::unique_ptr<Record>& rec); //case 1,2,4,5
        void undo(); 

    private:

        void push(std::unique_ptr<Record> rec);
        std::deque<std::unique_ptr<Record>> undo_stack;
        size_t max_capacity;


    };
  
}

#endif
#endif
