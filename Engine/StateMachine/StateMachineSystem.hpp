#pragma once

#include "ISystem.hpp"
#include "StateMachine.hpp"

namespace EngineFramework
{
    class StateMachineSystem final : public ISystem
    {
    public:
        StateMachineSystem() = default;
        ~StateMachineSystem() override = default;

        Engine::StateMachine& GetMachine() { return m_stateMachine; }
        const Engine::StateMachine& GetMachine() const { return m_stateMachine; }

    private:
        void Initialize() override;
        void Update(float dt) override;
        void PostUpdate(float dt) override
        {
            UNREFERENCED_PARAMETER(dt);
        }

    private:
        Engine::StateMachine m_stateMachine;
    };
}