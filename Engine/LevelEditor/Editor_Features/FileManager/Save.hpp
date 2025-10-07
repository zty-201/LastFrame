/*
*****************************************************************/
/*!
\file   Save.hpp
\author Tan Yee Tong
\par    yeetong.t\@digipen.edu
\date   September 2025
\brief
This file contains functions that saves the scene file that is currently being loaded
Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* *******************************************************************/


#if defined(PLATFORM_WINDOWS) && defined(_DEBUG)
#ifndef SAVE_LFEATURE_HPP
#define SAVE_LFEATURE_HPP
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

    class Save : public LFeature
    {
    public:

        Save();
        ~Save() override;

        void Initialize() override;
        void Run() override;
        void Close() override;

        bool SAVEFILE(std::string filename);


        static Save& GetInstance()
        {
            return *this_ptr;
        }

        static inline Save* this_ptr = nullptr;
    private:

        bool save_Archtypes();
        bool save_Instances(std::string filename);

    };



}




#endif
#endif
