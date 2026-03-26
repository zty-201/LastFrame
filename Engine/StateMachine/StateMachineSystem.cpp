#include "pch.hpp"
#include "StateMachineSystem.hpp"

#include "SplashState.hpp"
#include "MainMenuState.hpp"
#include "IntroCutsceneState.hpp"
#include "GameplayState.hpp"
#include "Audio.hpp"
#include "ScriptWrapper.hpp"

namespace EngineFramework
{
    StateMachineSystem* StateMachineSystem::this_instance = nullptr;
    std::string EngineFramework::StateMachineSystem::prev_state = "";

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

        // Pause to main menu
        m_stateMachine.AddTriggerTransition("Pause", "MainMenu", "ReturnToMenu");

        // Gameplay to main menu
        m_stateMachine.AddTriggerTransition("Gameplay", "Pause", "PauseGame");
        m_stateMachine.AddTriggerTransition("Pause", "Gameplay", "BacktoGame");

        //main menu and settings
        m_stateMachine.AddTriggerTransition("MainMenu", "Pause", "PauseGame");
        m_stateMachine.AddTriggerTransition("Pause", "MainMenu", "BacktoMainMenu");

        // Gameplay to main menu
        m_stateMachine.AddTriggerTransition("Gameplay", "EndCredit", "ToEndCredit");
        m_stateMachine.AddTriggerTransition("Pause", "EndCredit", "ToEndCredit");
        m_stateMachine.AddTriggerTransition("MainMenu", "EndCredit", "ToEndCredit");

        // EndCredit -> MainMenu (auto at finish, or manual)
        m_stateMachine.AddTriggerTransition("EndCredit", "MainMenu", "ReturnToMenu");

        m_started = false;

		// connect to the event dispatcher to handle OS-level pause signals (e.g., app going to background on mobile)
        event_dispatcher.sink<Pause_Event>().connect<&StateMachineSystem::HANDLE_OS_PAUSE>(this);
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
            prev_state = get_currentState_name();
        }

        std::string oldState = get_currentState_name();

        m_stateMachine.Update(dt);

        std::string newState = get_currentState_name();

        if (oldState != newState)
        {
            prev_state = oldState; // store where we came from

            LOGI("[StateMachineSystem] State changed: %s -> %s",
                prev_state.c_str(), newState.c_str());

            // Notify audio system of the new game state
            EngineFramework::AudioSystem::GetInstance().SetGameState(newState);

            // Sync Lua GAME_STATE global — Lua reads this, never writes it
            auto& lua = Engine::ScriptSystem::GetLuaState();

            if (newState == "Gameplay")
                lua["GAME_STATE"] = "GAME";
            else if (newState == "IntroCutscene")
                lua["GAME_STATE"] = "CUTSCENE";
            else if (newState == "Pause")
            {
            } // do nothing — keep GAME_STATE as is so ambience manager doesn't react
            else if (oldState == "Pause")
            {
            } // returning from pause — keep GAME_STATE as is
            else
                lua["GAME_STATE"] = "MENU";
        }

        // ----- Periodic state print every 5 seconds (debug) -----
        static float dbgTimer = 0.0f;
        dbgTimer += dt;
        if (dbgTimer >= 5.0f)
        {
            dbgTimer = 0.0f;
            LOGI("[StateMachineSystem] Current state: %s", newState.c_str());
        }
        
    }

	// This function is called when the OS sends a pause event (e.g., app is backgrounded on mobile)
    void StateMachineSystem::HANDLE_OS_PAUSE(Pause_Event& event)
    {
        // Only act if the OS is telling us the app is being backgrounded
        if (event.is_pause)
        {
            // Safety check: Only trigger Pause if we are currently in Gameplay!
            // This prevents the game from trying to pause while in the Main Menu or Splash screen.
            if (get_currentState_name() == "Gameplay")
            {
                LOGI("[StateMachineSystem] OS Pause detected. Triggering 'PauseGame' transition.");

                // This sets the trigger that the StateMachine checks during its next Update()
                m_stateMachine.SetTrigger("PauseGame");
            }
        }
    }

}