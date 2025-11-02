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