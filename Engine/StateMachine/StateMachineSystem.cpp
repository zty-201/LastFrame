#include "pch.hpp"
#include "StateMachineSystem.hpp"

#include "SplashState.hpp"
#include "MainMenuState.hpp"
#include "IntroCutsceneState.hpp"
#include "GameplayState.hpp"

namespace EngineFramework
{
    void StateMachineSystem::Initialize()
    {
        LOGI("[StateMachineSystem] Registering ordered states");

        m_stateMachine.AddState(std::make_unique<SplashState>());
        m_stateMachine.AddState(std::make_unique<MainMenuState>());
        m_stateMachine.AddState(std::make_unique<IntroCutsceneState>());
        m_stateMachine.AddState(std::make_unique<GameplayState>());

        // Splash -> Main Menu
        m_stateMachine.AddBoolTransition("Splash", "MainMenu", "SplashFinished", true);
        m_stateMachine.AddTriggerTransition("Splash", "MainMenu", "SkipSplash");

        // Main Menu -> Intro
        m_stateMachine.AddTriggerTransition("MainMenu", "IntroCutscene", "StartIntro");

        // Intro -> Gameplay
        m_stateMachine.AddBoolTransition("IntroCutscene", "Gameplay", "IntroCutsceneFinished", true);
        m_stateMachine.AddTriggerTransition("IntroCutscene", "Gameplay", "SkipIntro");

        m_started = false;
    }

    void StateMachineSystem::Update(float dt)
    {
        if (!m_started)
        {
            LOGI("[StateMachineSystem] Starting ordered flow at Splash");
            m_stateMachine.SetInitialState("Splash");
            m_started = true;
        }

        m_stateMachine.Update(dt);
    }
}