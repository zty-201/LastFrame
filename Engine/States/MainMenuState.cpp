#include "pch.hpp"
#include "MainMenuState.hpp"
#include "UISystem.hpp"

const std::string& MainMenuState::GetName() const
{
    return m_name;
}

void MainMenuState::OnEnter(Engine::StateMachine& machine)
{
    UNREFERENCED_PARAMETER(machine);

    LOGI("[State] Enter MainMenu");

    EngineFramework::UI_System::GetInstance().closeAllMenu();
    EngineFramework::UI_System::GetInstance().closeAllMenu(true); //clearing
    EngineFramework::UI_System::GetInstance().displayMenu("StartMenu");
}

void MainMenuState::OnExit(Engine::StateMachine& machine)
{
    UNREFERENCED_PARAMETER(machine);

    EngineFramework::UI_System::GetInstance().closeAllMenu();
    LOGI("[State] Exit MainMenu");
}

void MainMenuState::Update(Engine::StateMachine& machine, float dt)
{
    UNREFERENCED_PARAMETER(dt);
    UNREFERENCED_PARAMETER(machine);

    // Hook this to your Start button callback later.
    // For testing, you can temporarily auto-advance with a key or timer.

    // Example temporary debug trigger:
    // if (some start button pressed)
    // {
    //     LOGI("[State] MainMenu -> IntroCutscene");
    //     machine.SetTrigger("StartIntro");
    // }
}
void MainMenuState::Render(Engine::StateMachine& machine)
{
    UNREFERENCED_PARAMETER(machine);
}