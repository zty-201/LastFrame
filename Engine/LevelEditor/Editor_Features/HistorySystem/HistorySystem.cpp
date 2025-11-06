/*
*****************************************************************/
/*!
\file   HistorySystem.cpp
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
#include "pch.hpp"

//File management, JSON file parser/writer
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

//Entt reflection system
#include <entt/meta/meta.hpp>
#include <entt/meta/resolve.hpp>
//Entt ECS system
#include <entt/entity/registry.hpp>
#include "System_LevelEditor.hpp"

//Casting meta data to type.
#include "ReflectionRegister.hpp"
#include "System_GameObjectSystem.hpp"
#include "HistorySystem.hpp"
#include "InspectorPanel.hpp"
#include "GraphicsSystem.h"



namespace EngineFramework
{
    using namespace entt::literals;

    
    HistorySystem::HistorySystem(size_t value) {

        this_class = this;
        max_capacity = value;
        LOGI("Lfeature: HistorySystem CREATED");
    }
    HistorySystem::~HistorySystem() {

        undo_stack.clear();
    }
    void HistorySystem::Initialize() {

    }
    void HistorySystem::push(std::unique_ptr<Record> rec) {

        if (undo_stack.size() >= max_capacity)
            undo_stack.pop_front();

        undo_stack.push_back(std::move(rec));       
    }

    void HistorySystem::undo() {

        if (undo_stack.size() > 0) {

            undo_stack.back()->pop(); //this is usually value assignment/adding/removing entity or component based on the case
            undo_stack.pop_back();
        }        
    }

    std::unique_ptr<Record> HistorySystem::start_track(entt::entity e, std::string component, Record_Type type) {

        auto record = std::make_unique<Record_Component_Whole>();
        record->Type = type;
        record->entity = e;
        record->component = component;

        auto meta = entt::resolve(entt::hashed_string{ component.c_str() });
        if (!meta) {
            record->Type = Record_Type::INVALID;
            return record;
        }

        auto getref = meta.func(entt::hashed_string{ "get_ref" }.value());
        if (!getref) {
            record->Type = Record_Type::INVALID;
            return record;
        }

        entt::meta_any comp_ref = getref.invoke({}, e);
        if (!comp_ref) {
            record->Type = Record_Type::INVALID;
            return record;
        }

        for (auto&& [id, data] : meta.data())
        {
            std::string var_name = data.name();
            entt::meta_any value = data.get(comp_ref);

            if (!value) continue;
            record->variables[var_name] = std::move(value);

        }    
        return record;
    }


    std::unique_ptr<Record> HistorySystem::start_track(entt::entity e, std::string component, std::string var_name, Record_Type type) {

        auto record = std::make_unique<Record_Component_Single>();
        record->Type = type;
        record->entity = e;
        record->component = component;
        record->variable.first = var_name;

        auto meta = entt::resolve(entt::hashed_string{component.c_str() });
        if (!meta){
            record->Type = Record_Type::INVALID;
            return record;
        }

        auto getref = meta.func(entt::hashed_string{"get_ref"}.value());
        if (!getref){
            record->Type = Record_Type::INVALID;
            return record;
        }

        entt::meta_any comp_ref = getref.invoke({}, e);
        if (!comp_ref){
            record->Type = Record_Type::INVALID;
            return record;
        }

        auto data = meta.data(entt::hashed_string{ var_name.c_str() }.value());
        if (!data){
            record->Type = Record_Type::INVALID;
            return record;
        }

        entt::meta_any field_value = data.get(comp_ref);
        record->variable.second = std::move(field_value);

        return record;
    }

    
    std::unique_ptr<Record> HistorySystem::start_track(entt::entity e, Record_Type type) {

        auto record = std::make_unique<Record_Entity>();
        record->Type = type;
        record->entity = e;

        for (const auto& [type_id, meta] : entt::resolve()) {

            if (!is_meta_component(meta))
                continue;

            std::string component_name = meta.name();
            if (component_name.empty())
                continue;

            // does entity have this component?
            auto* storage = registry.storage(type_id);
            if (!storage || !storage->contains(e))
                continue;

            auto getref = meta.func(entt::hashed_string{ "get_ref" }.value());
            if (!getref)
                continue;

            entt::meta_any comp_ref = getref.invoke({}, e);
            if (!comp_ref)
                continue;
            
            for (auto&& [id, variable] : meta.data()) {

                std::string var_name = variable.name();
                if (var_name.empty())
                    continue;

                entt::meta_any value = variable.get(comp_ref);
                if (!value)
                    continue;

                record->components_variables[component_name][var_name] = std::move(value);

            }
        }
        return record;
    }


    void HistorySystem::end_track(std::unique_ptr<Record>& rec, const entt::meta_any& new_value) {

        auto* record = dynamic_cast<Record_Component_Single*>(rec.get());
        if (!record) return;
        if (record->Type == Record_Type::INVALID) return; //if it is record invalid, then do not push

        if (record->variable.second != new_value) {
            push(std::move(rec));
        }       
    }
    void HistorySystem::end_track(std::unique_ptr<Record>& rec) {
        push(std::move(rec));        
    }
 
    void Record_Component_Whole::pop() {

        if (Type == Record_Type::COMP_REMOVED){

            auto new_component = entt::resolve(entt::hashed_string{ component.c_str() });
            if (!new_component) return;

            entt::meta_any instance = new_component.construct();
            auto emplace = new_component.func("emplace"_hs);
            if (emplace) {
                emplace.invoke({}, entity, instance);
            }

            auto getref = new_component.func(entt::hashed_string{ "get_ref" }.value());
            if (!getref) return;

            entt::meta_any comp_ref = getref.invoke({}, entity);
            if (!comp_ref) return;

            for (auto&& [id, data] : new_component.data())
            {
                std::string var_name = data.name();
                auto it = variables.find(var_name);
                if (it == variables.end()) continue;               
                data.set(comp_ref, it->second);
            }


        }else if (Type == Record_Type::COMP_ADDED){

            auto component_to_remove = entt::resolve(entt::hashed_string{ component.c_str() });
            if (!component_to_remove) return;

            auto remove = component_to_remove.func("remove"_hs);
            if (remove) {
                entt::meta_any result = remove.invoke({}, entity);               
            }           
        }

    }
  
    void Record_Component_Single::pop() {

        if(Type != Record_Type::COMP_EDITED) return;

        auto meta = entt::resolve(entt::hashed_string{ component.c_str() });
        if (!meta) return;

        auto getref = meta.func("get_ref"_hs);
        if (!getref) return;

        entt::meta_any comp_ref = getref.invoke({}, entity);
        if (!comp_ref) return;

        auto data = meta.data(entt::hashed_string{ variable.first.c_str() }.value());
        if (!data) return;

        data.set(comp_ref, variable.second);
        
    }
  
    void Record_Entity::pop() {

        if (Type == Record_Type::ENTITY_ADDED) {

            if (!GameObjectSystem::GetRegistry().valid(entity)) {
                entity = GameObjectSystem::GetRegistry().create(entity);
            }
            
            for (auto& [component_name, variable_map] : components_variables)
            {
                auto meta_type = entt::resolve(entt::hashed_string{ component_name.c_str() });
                if (!meta_type) continue;

                entt::meta_any instance = meta_type.construct();

                auto emplace = meta_type.func("emplace"_hs);
                if (emplace) {
                    emplace.invoke({}, entity, instance);                   
                }

                auto getref_fn = meta_type.func("get_ref"_hs);
                if (!getref_fn) continue;

                entt::meta_any comp_ref = getref_fn.invoke({}, entity);
                if (!comp_ref) continue;

                for (auto& [field_name, stored_value] : variable_map)
                {
                    auto data_member = meta_type.data(entt::hashed_string{ field_name.c_str() });
                    if (!data_member) continue;

                    data_member.set(comp_ref, stored_value);
                }
            }           
        }
        else if (Type == Record_Type::ENTITY_REMOVED) {
           
            if (GameObjectSystem::GetRegistry().valid(entity)) {
                GameObjectSystem::GetRegistry().destroy(entity);
            }
        }
    }

    Record_Component_Whole::~Record_Component_Whole() {
        for (auto& [variable, value] : variables)
            value.reset();
        variables.clear();
    }

    Record_Component_Single::~Record_Component_Single() {
        variable.second.reset();
    }

    Record_Entity::~Record_Entity() {

        for (auto& [name, Component] : components_variables) {
            for (auto& [variable, value] : Component)
                value.reset();
        }
        components_variables.clear();
    }

}
#endif
