#pragma once

#include <string>

namespace Engine
{
    class StateMachine;

    class IState
    {
    public:
        virtual ~IState() = default;

        virtual const std::string& GetName() const = 0;

        virtual void OnEnter(StateMachine& /*machine*/) {}
        virtual void OnExit(StateMachine& /*machine*/) {}

        virtual void Update(StateMachine& /*machine*/, float dt) = 0;
        virtual void Render(StateMachine& /*machine*/) = 0;
    };
}