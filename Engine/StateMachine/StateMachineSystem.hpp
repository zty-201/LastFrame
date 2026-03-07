#pragma once

#include "ISystem.hpp"
#include "StateMachine.hpp"

namespace EngineFramework
{
    class StateMachineSystem : public ISystem
    {
    public:
        StateMachineSystem() = default;
        ~StateMachineSystem() override = default;

        Engine::StateMachine& GetMachine() { return m_stateMachine; }
        const Engine::StateMachine& GetMachine() const { return m_stateMachine; }

    private:
        void Initialize() override;
        void Update(float dt) override;
        //void PostUpdate(float dt) override {}

    private:
        Engine::StateMachine m_stateMachine;

        // Add this line
        bool m_started = false;
    };
}