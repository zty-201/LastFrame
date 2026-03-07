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