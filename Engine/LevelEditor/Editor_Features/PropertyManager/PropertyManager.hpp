/*****************************************************************/
/*!
\file   PropertyManager.hpp
\author Neo HuiZong
\par    neo.h\@digipen.edu
\date   October 2025
\brief
This file contains the declaration of the functions that are needed to
decide which entity to inspect for the inspector.
Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* *******************************************************************/

#include "pch.hpp"
#pragma once
#include "InspectorPanel.hpp"
#include "System_GameObjectSystem.hpp"
#include "LFeature.hpp"

namespace EngineFramework {
    class LFeature;

    class LFeature_PropertyManager final : public LFeature {
    public:
        LFeature_PropertyManager() = default;
        ~LFeature_PropertyManager() override = default;

        bool is_enabled = true; // Start 

        void Initialize() override;
        void Run() override; // Draw window each editor frame (when enabled)
        void Close() override;

        // Test trigger to spawn entity and add component and select
       static void Debug_SpawnTestEntityAndSelect();

    };
}