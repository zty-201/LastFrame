/*
*****************************************************************/
/*!
\file   AssetWindow.cpp
\author Tang Yeun, Dean
\par    y.tang\@digipen.edu
\date   September 2025
\brief
This file allows the user to find all game assets
Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* *******************************************************************/
#if defined(PLATFORM_WINDOWS) && defined(_DEBUG)

#include "pch.hpp"
#include "AssetWindow.hpp"
#include "Assets/System/AssetManager.hpp"
#include "FileDialog.hpp"
#include "HistorySystem.hpp"
#include "scene.h"
#include <cctype>
#include <system_error>

namespace EngineFramework
{
    namespace {

        // Any meta-registered component with emplace/get_ref is now eligible even when no instances contain them
        static bool is_meta_component(const entt::meta_type& meta) {
            if (!meta || !meta.is_class()) {
                return false;
            }
            using namespace entt::literals;
            return meta.func("emplace"_hs) && meta.func("get_ref"_hs);
        }

        static bool is_image_extension(const std::string& extension) {
            std::string lower = extension;
            std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) {
                return static_cast<char>(std::tolower(c));
            });
            return lower == ".png" || lower == ".jpg" || lower == ".jpeg";
        }
    }
	AssetWindow::AssetWindow()
	{
        this_class = this;
		rootAssetDirectory = currentDirectory = AssetManagerSystem::GetInstance().GetAssetFolderPrefix().string() + "Extern"; // Start at project asset root
	}

	AssetWindow::~AssetWindow()
	{
	}

	void AssetWindow::Initialize()
	{
        /*m_FolderIcon = LoadTextureFromFile("")*/
        is_window = false;   
        init_flags();
        init_extensions();

        /*
        * 
        */
        
	}

	void AssetWindow::Run()
	{
        //HistorySystem::GetInstance().start_track(LevelEditor::GetInstance().variables.selected_objectID, "MaterialComponent", Record_Type::COMP_ADDED);
        //HistorySystem::GetInstance().start_track(LevelEditor::GetInstance().variables.selected_objectID, "MaterialComponent", "rough");
        //HistorySystem::GetInstance().start_track(LevelEditor::GetInstance().variables.selected_objectID, Record_Type::ENTITY_ADDED);

        if (!is_enabled) return;

        if (is_window) {
            open_window();
        }

        if (ImGui::Begin("Asset Browser", &is_enabled))
        {
            // --- Header --- 
            ImGui::Text("Current: %s", currentDirectory.filename().string().c_str());
            ImGui::Separator();
            if (currentDirectory != rootAssetDirectory && currentDirectory.has_parent_path()) {
                if (ImGui::Button("Back")) {
                    currentDirectory = currentDirectory.parent_path();
                }
            }

            // Place Upload button on the far right
            float windowWidth = ImGui::GetContentRegionAvail().x;
            float buttonWidth = 100.0f;  // whatever size you want
            ImGui::SameLine(windowWidth - buttonWidth);

            if (ImGui::Button("Upload", ImVec2(buttonWidth, 0))) {
                std::string file = EngineFramework::OpenFileDialog("All Files\0*.*\0");
                if (!file.empty()) {
                    EngineFramework::CopyFileToProject(file, currentDirectory.string());
                }
            }

            if (ImGui::Button("Refresh", ImVec2(buttonWidth, 0))) {
                AssetManagerSystem::GetInstance().RemoveAllResources();
                AssetManagerSystem::GetInstance().LoadAllResources();
            }
            ImGui::SameLine();
            if (ImGui::Button("Link Component")) {
                is_window = true;
            }

            ImGui::Separator();
            

            // --- Layout ---
            static float padding = 5.0f;
            static float thumbnailSize = 72.0f;
            float cellSize = thumbnailSize + padding;

            float panelWidth = ImGui::GetContentRegionAvail().x;
            int columns = static_cast<int>(panelWidth / cellSize);
            if (columns < 1) columns = 1;

            ImGui::Columns(columns, 0, false);

            bool directoryChanged = false;
            std::filesystem::path newDirectory;

            // --- Draw folders ---
            for (auto& entry : std::filesystem::directory_iterator(currentDirectory))
            {
                if (entry.is_directory())
                {
                    std::string folderName = entry.path().filename().string();
                    ImGui::PushID(folderName.c_str());

                    if (ImGui::Button("Folder", { thumbnailSize, thumbnailSize })) {
                        directoryChanged = true;
                        newDirectory = entry.path();
                    }
                    ImGui::TextWrapped("%s", folderName.c_str());

                    ImGui::NextColumn();
                    ImGui::PopID();
                }
            }

            // --- Draw files ---
            for (auto& entry : std::filesystem::directory_iterator(currentDirectory))
            {
                if (!entry.is_directory())
                {
                    std::string fileName = entry.path().filename().string();
                    ImGui::PushID(fileName.c_str());

                    std::string extension = entry.path().extension().string();
                    std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c) {
                        return static_cast<char>(std::tolower(c));
                        });
                    bool isTextureFile = is_image_extension(extension);
                    unsigned int thumbnailId = isTextureFile ? GetThumbnailTexture(entry.path()) : 0;
                    ImTextureID previewHandle = thumbnailId ? static_cast<ImTextureID>(thumbnailId) : ImTextureID_Invalid;
                    ImTextureRef previewRef = (previewHandle != ImTextureID_Invalid) ? ImTextureRef(previewHandle) : ImTextureRef();

                    if (previewHandle != ImTextureID_Invalid) {
                        if (ImGui::ImageButton("##Thumbnail", previewRef, { thumbnailSize, thumbnailSize }, ImVec2(0, 1), ImVec2(1, 0))) {
                            selectedAsset = entry.path();
                        }
                    }
                    else {
                        if (ImGui::Button(extension.c_str(), { thumbnailSize, thumbnailSize })) {
                            selectedAsset = entry.path();
                        }
                    }

                    // Begin drag-drop source for all file types
                    if (ImGui::BeginDragDropSource())
                    {
                        // Payload type
                        std::string pathStr = entry.path().string();

                        //*****Set payload type for targets with specific requirements*****
                        const char* payloadType = "GENERAL";
                        if (extension == ".png" || extension == ".jpg" || extension == ".jpeg") {
                            payloadType = "Textures";
                        }
                        else if (extension == ".mp4" || extension == ".mov" || extension == ".mkv") {
                            payloadType = "Videos";
                        }

                        ImGui::SetDragDropPayload(payloadType, pathStr.c_str(), pathStr.size() + 1);

                        // Preview text + thumbnail when available
                        ImGui::TextUnformatted(fileName.c_str());
                        if (previewHandle != ImTextureID_Invalid) {
                            ImGui::Image(previewRef, ImVec2(50, 50), ImVec2(0, 1), ImVec2(1, 0));
                        }
                        ImGui::EndDragDropSource();
                    }


                    ImGui::TextWrapped("%s", fileName.c_str());

                    ImGui::NextColumn();
                    ImGui::PopID();
                }
            }

            if (directoryChanged) {
                currentDirectory = newDirectory;
            }

            ImGui::Columns(1);

            // --- Controls ---
            ImGui::SliderFloat("Thumbnail Size", &thumbnailSize, 32.0f, 256.0f);
            ImGui::SliderFloat("Padding Size", &padding, 0.0f, 24.0f);
        }
        ImGui::End();
	}

    void AssetWindow::open_window() {

        static bool open_popup= false;
        static std::string POPUP_NAME;
        static std::pair<std::string, std::string> component_variable_selected;

        if (open_popup) {
            ImGui::OpenPopup(POPUP_NAME.c_str()); //if edited, open popup
            open_popup = false;
        }

        if (ImGui::Begin("Manage Component Asset", &is_window)) {
            ImGui::TextWrapped("Select a componenent that is registered to entt:meta. Select it's variable that require asset from the asset browser. Include the file type that it accepts.");
            
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
            ImGui::TextWrapped("Variables without any file types selected will be marked red. It will not be seralised when saved.");
            ImGui::PopStyleColor();
            
            ImGui::Dummy(ImVec2(0.0f, 20.f));
            if (ImGui::Button("Choose Component")) {
                open_popup = true;
                POPUP_NAME = "Choose A Component";
            }
            ImGui::Dummy(ImVec2(0.0f, 20.f));
            for (const auto& [component, variables] : component_mapping) {
                
                ImGui::SeparatorText(component.c_str());
                ImGui::Dummy(ImVec2(0.0f, 20.f));

                for (const auto& [variable, tags] : variables) {

                    if (tags.empty()) {
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
                        ImGui::Text(variable.c_str());
                        ImGui::PopStyleColor();
                    }else{
                        ImGui::Text(variable.c_str());
                    }
                    
                    ImGui::SameLine();

                    std::string button_id = "Edit Tags##" + variable; // text before ## is visible, after ## is internal ID

                    if (ImGui::Button(button_id.c_str()))
                    {
                        ImGui::CloseCurrentPopup();
                        
                        POPUP_NAME = "Edit/Add Tags";
                        open_popup = true;
                        component_variable_selected.first = component;
                        component_variable_selected.second = variable;
                    }

                }
                ImGui::Dummy(ImVec2(0.0f, 20.f));
            }

        }
        ImGui::End();

        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(600, 720), ImGuiCond_Once);

        ImGui::SetNextWindowSize(ImVec2(600, 720), ImGuiCond_Once);
        if (ImGui::BeginPopupModal("Choose A Component", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove))
        {
            static entt::meta_type selected_type{};

            ImGui::BeginChild("LeftPanel", ImVec2(300, 600), true);
            {
                ImGui::TextWrapped("Select a new Component");
                ImGui::Dummy(ImVec2(0.0f, 20.f));
                ImGui::Separator();

                for (const auto& [type_id, meta] : entt::resolve()) {
                    if (!is_meta_component(meta))
                        continue;
                    (void)type_id;

                    // Get the registered type name (the string from .type("MeshComponent"))
                    const char* name = meta.name();
                    if (!name || name[0] == '\0')
                        continue;

                    if (ImGui::Selectable(name, selected_type == meta, ImGuiSelectableFlags_DontClosePopups)) {
                        selected_type = meta;
                    }
                }


            }
            ImGui::EndChild();

            
            ImGui::SameLine();

            // --- Right panel ---
            ImGui::BeginChild("RightPanel", ImVec2(0, 600), true);
            {
                
                if (selected_type) {

                    ImGui::Text("Choose the variable(s) to link");
                    ImGui::Separator();

                   
                    for (auto&& [id, variable] : selected_type.data()) {
                        
                        const char* name = variable.name();                            
                        ImGui::Checkbox(name, &flag(selected_type.name(), name));

                    }

                    

                }

            }
            ImGui::EndChild();

            ImGui::Dummy(ImVec2(0.0f, 20.f));
            if (ImGui::Button("Confirm and Close"))
            {
                ImGui::CloseCurrentPopup();
                open_popup = false;
                selected_type = entt::meta_type{};
                update_map(); //update component_map
            }

            ImGui::EndPopup();
        }

        ImGui::SetNextWindowSize(ImVec2(600, 600), ImGuiCond_Once);
        if (ImGui::BeginPopupModal("Edit/Add Tags", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove))
        {
            auto it = component_mapping.find(component_variable_selected.first);
            if (it != component_mapping.end()) {
                auto iter = it->second.find(component_variable_selected.second);
                if (iter != it->second.end()) {

                    static int current_item = -1; // none is selected
                    const char* preview = (current_item >= 0) ? valid_extensions[current_item].c_str() : "Choose a file extension";
                    ImGui::Dummy(ImVec2(0.0f, 20.f));
                    if (ImGui::BeginCombo("##ExtensionCombo", preview)) {
                        for (int n = 0; n < valid_extensions.size(); n++) {
                            bool is_selected = (current_item == n);
                            if (ImGui::Selectable(valid_extensions[n].c_str(), is_selected))
                                current_item = n;

                            if (is_selected)
                                ImGui::SetItemDefaultFocus();
                        }
                        ImGui::EndCombo();
                    }

                    if (ImGui::Button("Add")) {
                        //if it the tag is not repeated, then add)
                        if (current_item != -1 && check_extension_validity(valid_extensions[current_item])) {
                            iter->second.insert(valid_extensions[current_item]);
                            current_item = -1;
                        }                       
                    }
                    ImGui::Dummy(ImVec2(0.0f, 20.f));
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.90f, 0.20f, 0.20f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.95f, 0.35f, 0.35f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.80f, 0.15f, 0.15f, 1.0f));

                    ImGui::Dummy(ImVec2(0.0f, 20.f));
                    for (std::string tag : iter->second) {
                        ImGui::TextWrapped(tag.c_str());
                        ImGui::SameLine();
                        if (ImGui::Button(("-##" + tag).c_str()))
                        {
                            iter->second.erase(tag);                                                 
                            break;
                        }
                    }

                    ImGui::PopStyleColor(3);

                }
            }
            ImGui::Dummy(ImVec2(0.0f, 20.f));
           
            if (ImGui::Button("Confirm"))
            {               
                ImGui::CloseCurrentPopup();
                open_popup = false;                              
            }

            ImGui::EndPopup();
        }


    }

    bool AssetWindow::check_extension_validity(std::string tag) {
        
        for (std::string extension: valid_extensions) {
            if (extension == tag) return true;
        }
        return false;
    }
    void AssetWindow::update_map() {
       
        for (const auto& [component, variables] : component_validity_flags) {
            
            for (const auto& [variable, flag] : variables) {
                if (flag) {
                    //2 cases
                    //case 1: newly added
                    //case2: based on old value (carry forward)
                    std::vector<std::string> tags{};
                    if (!get_tags(component,variable, tags)) {                        
                        component_mapping[component][variable] = std::set<std::string>{};
                    }                   
                    
                }
                else {
                    //2 cases
                    //case 1: removed if it exists
                    //case2: based on old value (carry forward)

                    std::vector<std::string> tags{};
                    if (get_tags(component, variable, tags)) {
                        //case 1, do not care about case 2

                        auto it = component_mapping.find(component);
                        if (it != component_mapping.end()) {
                            auto& iter = it->second;
                            iter.erase(variable);

                            // if component has no variables left, erase it too
                            if (iter.empty()) {
                                component_mapping.erase(it);
                            }
                        }
                    }
                }
            }
    }



    }

    // To get the thumbnail texture as it is the cache entry point
    // Given a file path it will query the file last write time and look up to the existing entry within the cache
    // If the cache entry is valid, it will return the stored GL texture id, otherwise it deletes the old GL texture, remove entry and calls for LoadThumbnail to rebuild a new one
    // Then store the new entry back in to the cache before returning the texture id
    unsigned int AssetWindow::GetThumbnailTexture(const std::filesystem::path& path) {
        std::error_code ec;
        auto writeTime = std::filesystem::last_write_time(path, ec);
        if (ec) {
            return 0;
        }
        // we just access it from the entt meta no need to store in local cache as 
		// lookup time is around same and we need to ensure the data is always up to date,
        // otherwise we need to monitor file change and update the local cache accordingly which is more complex
        const std::string key = path.stem().string();
        using E_AM = EngineFramework::AssetManagerSystem;
        xresource::full_guid texture_guid = E_AM::GetFullGuid(E_AM::TEXTURE_GUID, key);
        auto textureId = E_AM::GetResource<Scene::Texture>(texture_guid);
        if (textureId) {
            GLuint texId = textureId->id;
            return texId;
        }

        return 0;
    }


    /*
    * return false if the component's variable does not exists in the component mapping
    */
    bool AssetWindow::get_tags(const std::string& component, const std::string& variable, std::vector<std::string> &tags) {

        auto it = component_mapping.find(component);
        if (it == component_mapping.end()) {
            return false;
        }

        auto iter = it->second.find(variable);
        if (iter == it->second.end())
            return false;

        std::vector<std::string> all_tags{};
        for (std::string tag: iter->second) {
            all_tags.push_back(tag);
        }
        tags = all_tags;

        return true;
    }

    /*
        have to ensure that whatever you are passing is in the map
    */

    void AssetWindow::init_extensions() {

        valid_extensions = {
            // --- Images ---
            ".png", ".jpg", ".jpeg",

            // --- Audio ---
            ".wav", ".mp3",

            // --- 3D Models / Meshes ---
            ".obj",

            // --- Text / Config / Data ---
            ".txt", ".json", ".lua",

            // --- Shaders / Code ---
            ".vert", ".frag", ".comp", ".glsl", ".shader",

            // --- Fonts ---
            ".ttf", ".otf", ".fnt",

            // --- Video ---
            ".mp4", ".mov", ".mkv"
        };

    }
    void AssetWindow::init_flags() {

        /*
            contains all components and variable in entt::meta, all flags are false
        */
        
        for (const auto& [type_id, meta] : entt::resolve()) {
            if (!is_meta_component(meta))
                continue;
            (void)type_id;

            const char* name = meta.name();
            if (!name || name[0] == '\0')
                continue;

            for (auto&& [id, variable] :meta.data()) {

                const char* var_name = variable.name();

                component_validity_flags[name][var_name] = false;

            }

        }

        /*
        *   deseralise file that contains tags, then turn the selected flags  = true
        */

        deseralise();
        for (const auto& [component, variables] : component_mapping) {

            auto it = component_validity_flags.find(component);
            if (it != component_validity_flags.end()) {

                for (const auto& [variable, tags] : variables) {
                    auto iter = it->second.find(variable);
                    if (iter != it->second.end()) {
                        iter->second = true;
                    }
                }
            }
                
        }

    }

    void AssetWindow::seralise() {

        //STORE DATA FROM KEY_MAPPING_DATA TO JSON FILE
        //logging purpose, does not key data
        nlohmann::json data;
        data["component_mapping"] = nlohmann::json::array();

        // Loop through outer map (component → variables)
        for (const auto& [component, variables] : component_mapping) {
            // Loop through inner map (variable → tags)
            for (const auto& [variable, tags] : variables) {
                //only write those componet-variable where extensions are included.
                if (!tags.empty()) {
                    nlohmann::json entry;
                    entry["component"] = component;
                    entry["variable"] = variable;
                    entry["tags"] = tags; // vector<string> auto converts to JSON array

                    data["component_mapping"].push_back(entry);
                }
                
            }
        }

        std::string path = AssetManagerSystem::GetAssetFolderPrefix().string() + "Extern/LevelEditor/component_mapping.json";

        std::ofstream file(path);
        if (!file.is_open()) {
            LOGE("Failed to open file for writing: %s", path.c_str());
            return;
        }

        file << data.dump(4); // pretty-print with indentation
        file.close();


    }

    void AssetWindow::deseralise() {

        std::string path = AssetManagerSystem::GetAssetFolderPrefix().string() + "Extern/LevelEditor/component_mapping.json";

        std::ifstream file(path);
        if (!file.is_open()) {
            LOGE("Failed to open file for reading: %s", path.c_str());
            return;
        }

        nlohmann::json data;
        file >> data;
        file.close();

        component_mapping.clear();

        // Ensure expected JSON structure
        if (!data.contains("component_mapping") || !data["component_mapping"].is_array())
            return;

        for (const auto& entry : data["component_mapping"]) {
            // Extract component, variable, and tags
            std::string component = entry.value("component", "");
            std::string variable = entry.value("variable", "");
            std::set<std::string> tags;

            if (entry.contains("tags") && entry["tags"].is_array()) {
                for (const auto& t : entry["tags"]) {
                    if (t.is_string())
                        tags.insert(t.get<std::string>());
                }
            }

            // Only insert valid entries
            if (!component.empty() && !variable.empty()) {
                component_mapping[component][variable] = tags;
            }
        }

    }


    const bool& AssetWindow::flag(const std::string& component, const std::string& variable) const{

        auto it = component_validity_flags.find(component);
        if (it == component_validity_flags.end()) {
            throw std::out_of_range("No such component or variable: " + variable + " in: " + component);
        }

        auto iter = it->second.find(variable);
        if (iter == it->second.end())
            throw std::out_of_range("No such component or variable: " + variable + " in: " + component);

        return iter->second;

    }

    bool& AssetWindow::flag(const std::string& component, const std::string& variable) {

        auto it = component_validity_flags.find(component);
        if (it == component_validity_flags.end()) {
            throw std::out_of_range("No such component or variable: " + variable + " in: " + component);
        }

        auto iter = it->second.find(variable);
        if (iter == it->second.end())
            throw std::out_of_range("No such component or variable: " + variable + " in: " + component);


        return iter->second;

    }

    void AssetWindow::sample_drag_drop() {

        if (ImGui::BeginDragDropTarget())
        {
            std::vector<std::string> tags; //a reference

            //component name, variable name, 
            //get tags return false if whatever we are looking for is not in the component_mapping map
            //component_mapping map contains the elements (and the extension that they accepts) that requires asset browser
            if (get_tags("MeshComponent", "indexCount", tags)) {

                for (const auto& type : tags)
                {
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(type.c_str()))
                    {
                        const char* droppedPath = static_cast<const char*>(payload->Data);

                        std::cout << "filedropped: " << droppedPath << std::endl;

                        //.... change the variable value to whatever ...

                        break; 
                    }
                }
            }

            ImGui::EndDragDropTarget();
        }
    }

	void AssetWindow::Close()
	{
	}
}
#endif
