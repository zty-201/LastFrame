#include "pch.hpp"
#include "StateMachineSystem.hpp"

#include "SplashState.hpp"
#include "MainMenuState.hpp"
#include "IntroCutsceneState.hpp"
#include "GameplayState.hpp"

namespace EngineFramework
{
    StateMachineSystem* StateMachineSystem::this_instance = nullptr;

    StateMachineSystem::StateMachineSystem()
    {
        this_instance = this;
    }

    StateMachineSystem& StateMachineSystem::GetInstance()
    {
        return *this_instance;
    }

    void StateMachineSystem::Initialize()
    {
        LOGI("[StateMachineSystem] Registering ordered states");

        m_stateMachine.AddState(std::make_unique<SplashState>());
        m_stateMachine.AddState(std::make_unique<MainMenuState>());
        m_stateMachine.AddState(std::make_unique<IntroCutsceneState>());
        m_stateMachine.AddState(std::make_unique<GameplayState>());
        m_stateMachine.AddState(std::make_unique<PauseState>());
        m_stateMachine.AddState(std::make_unique<EndCreditState>());
        //


        // Splash -> Main Menu
        m_stateMachine.AddBoolTransition("Splash", "MainMenu", "SplashFinished", true);
        m_stateMachine.AddTriggerTransition("Splash", "MainMenu", "SkipSplash");

        // Main Menu -> Intro
        m_stateMachine.AddTriggerTransition("MainMenu", "IntroCutscene", "StartIntro");

        // Intro -> Gameplay
        m_stateMachine.AddBoolTransition("IntroCutscene", "Gameplay", "IntroCutsceneFinished", true);
        m_stateMachine.AddTriggerTransition("IntroCutscene", "Gameplay", "SkipIntro");

        // Gameplay to main menu
        m_stateMachine.AddTriggerTransition("Gameplay", "MainMenu", "ReturnToMenu"); //should be removed, leave here for now

        // Gameplay to main menu
        m_stateMachine.AddTriggerTransition("Pause", "MainMenu", "ReturnToMenu");

        // Gameplay to main menu
        m_stateMachine.AddTriggerTransition("Gameplay", "Pause", "PauseGame");
        m_stateMachine.AddTriggerTransition("Pause", "Gameplay", "BacktoGame");

        // Gameplay to main menu
        m_stateMachine.AddTriggerTransition("Gameplay", "EndCredit", "ToEndCredit");
        m_stateMachine.AddTriggerTransition("Pause", "EndCredit", "ToEndCredit");
        m_stateMachine.AddTriggerTransition("MainMenu", "EndCredit", "ToEndCredit");


        m_started = false;
    }

    void StateMachineSystem::Update(float dt)
    {
        if (!m_started)
        {
            if (!m_stateMachine.GetCurrentState())
            {
                LOGI("[StateMachineSystem] Starting ordered flow at Splash");
                m_stateMachine.SetInitialState("Splash");
            }
            m_started = true;
        }

        m_stateMachine.Update(dt);
    }

}