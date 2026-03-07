#pragma once

#include "IState.hpp"
#include "StateMachine.hpp"
#include <string>

class IntroCutsceneState : public Engine::IState
{
public:
    IntroCutsceneState() = default;

    const std::string& GetName() const override;

    void OnEnter(Engine::StateMachine& machine) override;
    void OnExit(Engine::StateMachine& machine) override;
    void Update(Engine::StateMachine& machine, float dt) override;
    void Render(Engine::StateMachine& machine) override;

private:
    std::string m_name = "IntroCutscene";
};