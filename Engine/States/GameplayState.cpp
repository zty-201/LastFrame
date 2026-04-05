#include "pch.hpp"
#include "GameplayState.hpp"
#include "UISystem.hpp"

const std::string& GameplayState::GetName() const
{
    return m_name;
}

void GameplayState::OnEnter(Engine::StateMachine& machine)
{
    UNREFERENCED_PARAMETER(machine);

    LOGI("[State] Enter Gameplay");

    auto& ui = EngineFramework::UI_System::GetInstance();
    ui.closeAllMenu();
    ui.resumeActive_Menus();

    // Put your gameplay scene/runtime-start logic here.
#if defined(PLATFORM_ANDROID)
    
    ui.displayMenu_InvisibleLayer("Android_Inventory_Buttons");
    ui.displayMenu("AndroidInputOverlay");
        
    
#endif
}

void GameplayState::OnExit(Engine::StateMachine& machine)
{
    UNREFERENCED_PARAMETER(machine);
    auto& ui = EngineFramework::UI_System::GetInstance();
    ui.closeAllMenu(true);
    LOGI("[State] Exit Gameplay");
}

void GameplayState::Update(Engine::StateMachine& machine, float dt)
{
    UNREFERENCED_PARAMETER(machine);
    UNREFERENCED_PARAMETER(dt);
}

void GameplayState::Render(Engine::StateMachine& machine)
{
    UNREFERENCED_PARAMETER(machine);
}




const std::string& PauseState::GetName() const
{
    return m_name;
}

void PauseState::OnEnter(Engine::StateMachine& machine)
{
    UNREFERENCED_PARAMETER(machine);

    auto& ui = EngineFramework::UI_System::GetInstance();
    ui.closeAllMenu();
    ui.displayMenu("PauseMenu");

    // Trigger complete audio silence
    Pause_Event pause_event;
    pause_event.is_pause = true;
    event_dispatcher.trigger<Pause_Event>(std::move(pause_event));

    LOGI("[State] Enter Pause state");
}

void PauseState::OnExit(Engine::StateMachine& machine)
{
    UNREFERENCED_PARAMETER(machine);

    auto& ui = EngineFramework::UI_System::GetInstance();
    ui.closeAllMenu();

    // Wake the audio back up
    Pause_Event pause_event;
    pause_event.is_pause = false;
    event_dispatcher.trigger<Pause_Event>(std::move(pause_event));

    LOGI("[State] Exit Pause state");
}

void PauseState::Update(Engine::StateMachine& machine, float dt)
{
    UNREFERENCED_PARAMETER(machine);
    UNREFERENCED_PARAMETER(dt);
}

void PauseState::Render(Engine::StateMachine& machine)
{
    UNREFERENCED_PARAMETER(machine);
}

//

const std::string& EndCreditState::GetName() const
{
    return m_name;
}

void EndCreditState::OnEnter(Engine::StateMachine& machine)
{
    UNREFERENCED_PARAMETER(machine);

    LOGI("[State] Enter EndCredit state");
    auto& ui = EngineFramework::UI_System::GetInstance();

    ui.StartCreditsRoll("CreditsMenu", 120.0f, 5.f, true);
}

void EndCreditState::OnExit(Engine::StateMachine& machine)
{
    UNREFERENCED_PARAMETER(machine);
    LOGI("[State] Exit EndCredit state");

    auto& ui = EngineFramework::UI_System::GetInstance();
    ui.StopCreditsRoll(true);
}

void EndCreditState::Update(Engine::StateMachine& machine, float dt)
{
    UNREFERENCED_PARAMETER(machine);
    UNREFERENCED_PARAMETER(dt);
}
void EndCreditState::Render(Engine::StateMachine& machine)
{
    UNREFERENCED_PARAMETER(machine);
}