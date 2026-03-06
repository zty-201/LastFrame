#include "pch.hpp"
#include "StateMachineSystem.hpp"

#include "GameplayState.hpp"
#include "SplashState.hpp"

namespace EngineFramework
{
    void StateMachineSystem::Initialize()
    {
        m_stateMachine.AddState(std::make_unique<SplashState>());
        m_stateMachine.AddState(std::make_unique<GameplayState>());

        m_stateMachine.AddBoolTransition("Splash", "Gameplay", "SplashFinished", true);
        m_stateMachine.AddTriggerTransition("Splash", "Gameplay", "SkipSplash");

        m_stateMachine.SetInitialState("Splash");
    }

    void StateMachineSystem::Update(float dt)
    {
        m_stateMachine.Update(dt);
    }
}