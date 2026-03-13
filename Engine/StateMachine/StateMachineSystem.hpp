#pragma once

#include "ISystem.hpp"
#include "StateMachine.hpp"

namespace EngineFramework
{
    class StateMachineSystem : public ISystem
    {
    public:
        StateMachineSystem();
        ~StateMachineSystem() override = default;

        static StateMachineSystem& GetInstance();

        Engine::StateMachine& GetMachine() { return m_stateMachine; }
        const Engine::StateMachine& GetMachine() const { return m_stateMachine; }

        //bind this scripting.cpp
        std::string get_currentState_name() const {

            return m_stateMachine.GetCurrentStateName();
        }

    private:
        void Initialize() override;
        void Update(float dt) override;
        void PostUpdate(float dt) override
        {
            UNREFERENCED_PARAMETER(dt);
        }

    private:
        static StateMachineSystem* this_instance;

        Engine::StateMachine m_stateMachine;
        bool m_started = false;
    };
}