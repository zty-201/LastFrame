#pragma once

#include "IState.hpp"
#include "StateMachine.hpp"
#include <string>

class SplashState : public Engine::IState
{
public:
    SplashState() = default;

    const std::string& GetName() const override;

    void OnEnter(Engine::StateMachine& machine) override;
    void OnExit(Engine::StateMachine& machine) override;
    void Update(Engine::StateMachine& machine, float dt) override;
    void Render(Engine::StateMachine& machine) override;

private:
    std::string m_name = "Splash";
    float m_timer = 0.0f;
    float m_duration = 2.5f;
};