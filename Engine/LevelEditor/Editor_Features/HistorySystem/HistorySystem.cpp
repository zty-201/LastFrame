/*
*****************************************************************/
/*!
\file   HistorySystem.cpp
\author Tan Yee Tong
\par    yeetong.t\@digipen.edu
\date   September 2025
\brief
This file runs the Entity based history system of the level editor.
The undo/redo of components and its' respective fields work universally for all components so long as they are
registered properly. For example the data members are to be exposed via the reflectionregister.cpp so that it can be copied and replays etc.
Components without data members or components that are tagged with RequiresSerializedHistoryTag will fall back to the JSON capture path (Record_Component_Serialized).
Only cases that it cannot cover are components that have not been reflected at all for example no meta entry or fields whose types are not wired into reflection serialization helpers.
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

    // Reuse SetJsonWithMetaAny/SetMetaAnyWithJson from reflection module to serialize and deserialize components
    // Check for the RequiresSerializedHistoryTag, if dont have then treat any meta type with no data members as serialized
    // Hence force the empty components (SceneRenderPass/SkyBboxRenderPass etc) to use json-based history records
    namespace {
        bool requires_serialized_history(const entt::meta_type& meta) {
            if (!meta) {
                return false;
            }

            const auto tag = static_cast<const ::EngineFramework::RequiresSerializedHistoryTag*>(meta.custom());
            if (tag) {
                return true;
            }

            auto data_range = meta.data();
            return data_range.begin() == data_range.end();
        }

        std::string serialize_component(entt::entity e, const entt::meta_type& meta) {
            auto get = meta.func("get"_hs);
            if (!get) {
                return {};
            }
            entt::meta_any data = get.invoke({}, e);
            if (!data) {
                return {};
            }

            nlohmann::json json = nlohmann::json::object();
            for (auto&& [id, member] : meta.data()) {
                ::EngineFramework::SetJsonWithMetaAny(json, member, data.get(id));
            }
            return json.dump();
        }

        // Clear the old component instance, re emplaced it and report the success back to caller
        bool deserialize_component(entt::entity e, const entt::meta_type& meta, const std::string& payload) {
            if (!meta) {
                return false;
            }

            entt::meta_any instance = meta.construct();
            if (!payload.empty()) {
                nlohmann::json json = nlohmann::json::parse(payload);
                for (auto&& [id, member] : meta.data()) {
                    auto it = json.find(member.name());
                    if (it != json.end()) {
                        ::EngineFramework::SetMetaAnyWithJson(instance, member, *it);
                    }
                }
            }
            if (auto remove_fn = meta.func("remove"_hs)) {
                remove_fn.invoke({}, e);
            }
            if (auto emplace = meta.func("emplace"_hs)) {
                entt::meta_any result = emplace.invoke({}, e, static_cast<const void*>(&instance));
                if (!result) {
                    return true;
                }
                const auto type = result.type();
                if (type && type == entt::resolve<bool>()) {
                    return result.cast<bool>();
                }
                return true;
            }
            return false;
        }
    }

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

    // Always append the latest record to undo_stack and then clear redo_stack so redo never crosses new branch in history
    void HistorySystem::push(std::unique_ptr<Record> rec) {
        if (!rec) {
            return;
        }

        if (undo_stack.size() >= max_capacity)
            undo_stack.pop_front();

        redo_stack.clear();
        undo_stack.push_back(std::move(rec));       
    }

    // Always pop the nmewest record off undo_stack, then call for record pop so the action is played backward
    // Then call for record invert so the same object can now be played in opposite direction
    // Then push this record onto redo stack
    void HistorySystem::undo() {
        if (undo_stack.empty()) {
            return;
        }
        auto rec = std::move(undo_stack.back());
        undo_stack.pop_back();
        rec->pop();
        rec->invert();
        redo_stack.push_back(std::move(rec));

        //if (undo_stack.size() > 0) {

        //    undo_stack.back()->pop(); //this is usually value assignment/adding/removing entity or component based on the case
        //    undo_stack.pop_back();
        //}        
    }

    // Similar function as how undo works but functionality is the opposite of redo
    void HistorySystem::redo() {
        if (redo_stack.empty()) {
            return;
        }
        auto rec = std::move(redo_stack.back());
        redo_stack.pop_back();
        rec->pop();
        rec->invert();
        undo_stack.push_back(std::move(rec));
    }

    std::unique_ptr<Record> HistorySystem::start_track(entt::entity e, std::string component, Record_Type type) {
        auto meta = entt::resolve(entt::hashed_string{ component.c_str() });
        if (!meta) {
            auto rec = std::make_unique<Record_Component_Whole>();
            rec->Type = Record_Type::INVALID;
            return rec;
        }

        if (requires_serialized_history(meta)) {
            auto rec = std::make_unique<Record_Component_Serialized>();
            rec->Type = type;
            rec->entity = e;
            rec->component = component;
            rec->json_payload = serialize_component(e, meta);
            return rec;
        }

        auto record = std::make_unique<Record_Component_Whole>();
        record->Type = type;
        record->entity = e;
        record->component = component;

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
        record->variable_name = var_name;

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
        record->before_value = std::move(field_value);

        return record;
    }

    
    std::unique_ptr<Record> HistorySystem::start_track(entt::entity e, Record_Type type) {

        auto record = std::make_unique<Record_Entity>();
        record->Type = type;
        record->entity = e;
        auto& level_registry = GameObjectSystem::GetRegistry();

        for (const auto& [type_id, meta] : entt::resolve()) {

            if (!is_meta_component(meta))
                continue;

            std::string component_name = meta.name();
            if (component_name.empty())
                continue;

            // does entity have this component?
            auto* storage = level_registry.storage(type_id);
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
        if (!record) {
            rec.reset();
            return;
        }
        if (record->Type == Record_Type::INVALID) {
            rec.reset();
            return; //if it is record invalid, then do not push
        }

        record->after_value = new_value;

        if (record->before_value != record->after_value) {
            push(std::move(rec));
        } else {
            rec.reset();
        }
    }
    void HistorySystem::end_track(std::unique_ptr<Record>& rec) {
        push(std::move(rec));        
    }
    
    // To replay the stored action when undoing or redoing a component add/remove actions
    void Record_Component_Whole::pop() {

        // When a component is removed, in order to replay it, reconstruct the meta any then pass the pointer to the instance back into the registered emplace function
        if (Type == Record_Type::COMP_REMOVED){

            auto new_component = entt::resolve(entt::hashed_string{ component.c_str() });
            if (!new_component) {
                return;
            }

            entt::meta_any instance = new_component.construct();
            auto emplace = new_component.func("emplace"_hs);
            if (emplace) {
                // Clone the captured instance back in to the registry when undoing a component that has been removed
                emplace.invoke({}, entity, static_cast<const void*>(&instance));
            }

            auto getref = new_component.func(entt::hashed_string{ "get_ref" }.value());
            if (!getref) {
                return;
            }

            entt::meta_any comp_ref = getref.invoke({}, entity);
            if (!comp_ref) {
                return;
            }

            for (auto&& [id, data] : new_component.data())
            {
                std::string var_name = data.name();
                auto it = variables.find(var_name);
                if (it == variables.end()) continue;               
                data.set(comp_ref, it->second);
            }


        }
        // If component is added, the opposite would be to call for the registered remove function so the component will disappears
        else if (Type == Record_Type::COMP_ADDED){

            auto component_to_remove = entt::resolve(entt::hashed_string{ component.c_str() });
            if (!component_to_remove) return;

            auto remove = component_to_remove.func("remove"_hs);
            if (remove) {
                entt::meta_any result = remove.invoke({}, entity);               
            }           
        }

    }

    // Used to flip the recorded action so the same record can travel between undo and redo stacks
    void Record_Component_Whole::invert() {
        if (Type == Record_Type::COMP_ADDED) {
            Type = Record_Type::COMP_REMOVED;
        } else if (Type == Record_Type::COMP_REMOVED) {
            Type = Record_Type::COMP_ADDED;
        }
    }
    
    // To handle single field editing undo/redo
    void Record_Component_Single::pop() {

        if(Type != Record_Type::COMP_EDITED) return;

        auto meta = entt::resolve(entt::hashed_string{ component.c_str() });
        if (!meta) return;

        auto getref = meta.func("get_ref"_hs);
        if (!getref) return;

        entt::meta_any comp_ref = getref.invoke({}, entity);
        if (!comp_ref) return;

        auto data = meta.data(entt::hashed_string{ variable_name.c_str() }.value());
        if (!data) {
            return;
        }

        if (!before_value) {
            return;
        }

        data.set(comp_ref, before_value);
        
    }

    void Record_Component_Single::invert() {
        if (Type != Record_Type::COMP_EDITED) return;
        std::swap(before_value, after_value);
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

    void Record_Entity::invert() {
        if (Type == Record_Type::ENTITY_ADDED) {
            Type = Record_Type::ENTITY_REMOVED;
        } else if (Type == Record_Type::ENTITY_REMOVED) {
            Type = Record_Type::ENTITY_ADDED;
        }
    }

    Record_Component_Whole::~Record_Component_Whole() {
        for (auto& [variable, value] : variables)
            value.reset();
        variables.clear();
    }

    Record_Component_Single::~Record_Component_Single() {
        before_value.reset();
        after_value.reset();
    }

    Record_Entity::~Record_Entity() {

        for (auto& [name, Component] : components_variables) {
            for (auto& [variable, value] : Component)
                value.reset();
        }
        components_variables.clear();
    }

    // Take the return value and logs with LOGE if tagged component fails to restore
    void Record_Component_Serialized::pop() {
        auto meta = entt::resolve(entt::hashed_string{ component.c_str() });
        if (!meta) {
            return;
        }

        if (Type == Record_Type::COMP_REMOVED) {
            const bool restored = deserialize_component(entity, meta, json_payload);
            if (!restored) {
                LOGE("HistorySystem: failed to restore component '%s' on entity 0x%08X", component.c_str(), static_cast<unsigned>(entt::to_integral(entity)));
            }
        } else if (Type == Record_Type::COMP_ADDED) {
            auto remove = meta.func("remove"_hs);
            if (remove) {
                remove.invoke({}, entity);
            }
        }
    }

    void Record_Component_Serialized::invert() {
        if (Type == Record_Type::COMP_REMOVED) {
            Type = Record_Type::COMP_ADDED;
        } else if (Type == Record_Type::COMP_ADDED) {
            Type = Record_Type::COMP_REMOVED;
        }
    }

}
#endif
