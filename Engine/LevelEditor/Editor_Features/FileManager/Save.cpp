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

#include "Save.hpp"
//#include <iostream>

namespace EngineFramework
{
    Save::Save() {

        this_ptr = this;
        LOGI("Lfeature: Save CREATED");
    }
    Save::~Save() {

    }
    void Save::Initialize() {

    }
    void Save::Run() {

        static char inputBuffer[128] = "";
        const char* POPUP_NAME = "FILE Save As";

        static bool to_open = false;

        //only open once until it closes
        if (is_enabled && !to_open) {
            ImGui::OpenPopup(POPUP_NAME);
            to_open = true;
        }

        if (ImGui::BeginPopupModal(POPUP_NAME, NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
           
            ImGui::InputTextWithHint("##InputField", "Save file as...", inputBuffer, IM_ARRAYSIZE(inputBuffer));
            
            std::string str_buffer = std::string(inputBuffer);
            ImGui::Text("(*please exclude all spaces and include the extension*)");

            if (str_buffer.empty()) {

                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
                ImGui::Text("*Please provide a file name");
                ImGui::PopStyleColor();
            }

            ImGui::Spacing();

            if (ImGui::Button("Cancel"))
            {
                ImGui::CloseCurrentPopup();
                is_enabled = false;
                to_open = false;
            }
            ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - ImGui::GetStyle().FramePadding.x - 60);

            if (ImGui::Button("Save"))
            {
                //only save if the filename is not empty
                if (!str_buffer.empty()) {

                    printf("User entered: %s\n", inputBuffer);
                    ImGui::CloseCurrentPopup();

                    if (!SAVEFILE(str_buffer)) {
                        LOGI("Error saving file. File not saved\n");
                    }
                    is_enabled = false;
                    to_open = false;
                }
                
            }

            ImGui::EndPopup();
        }

        
    }
    void Save::Close() {

    }

    bool Save::SAVEFILE(std::string filename){

        if (filename.empty()) {
            return false;
        }

        std::string path = "Extern/Scenes/" + filename;
        std::ofstream Out_File(path);

        if (!Out_File.is_open()) return false;
        
        
       
        std::stringstream ss;

        /*
            Seralise here
            
        */

        ss << "Note that seralise is not done yet. This is just a dummy file to demo file saving. Do not data pharse this file\n";

        Out_File << ss.str(); // Output the contents of the stringstream
        Out_File.close();

        return true;
       
    }

}
#endif
