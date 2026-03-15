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

    // Put your gameplay scene/runtime-start logic here.
#if defined(PLATFORM_ANDROID)
    ui.displayMenu_InvisibleLayer("Android_Inventory_Buttons");
    ui.displayMenu("AndroidInputOverlay");
#endif
}

void GameplayState::OnExit(Engine::StateMachine& machine)
{
    UNREFERENCED_PARAMETER(machine);
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

    LOGI("[State] Enter Pause state");
}

void PauseState::OnExit(Engine::StateMachine& machine)
{
    UNREFERENCED_PARAMETER(machine);
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
    ui.closeAllMenu();
    ui.displayMenu("CreditsMenu");
}

void EndCreditState::OnExit(Engine::StateMachine& machine)
{
    UNREFERENCED_PARAMETER(machine);
    LOGI("[State] Exit EndCredit state");
    auto& ui = EngineFramework::UI_System::GetInstance();
    ui.closeAllMenu();
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