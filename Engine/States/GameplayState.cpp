#include "pch.hpp"
#include "GameplayState.hpp"

const std::string& GameplayState::GetName() const
{
    return m_name;
}

void GameplayState::OnEnter(Engine::StateMachine& machine)
{
    UNREFERENCED_PARAMETER(machine);
}

void GameplayState::OnExit(Engine::StateMachine& machine)
{
    UNREFERENCED_PARAMETER(machine);
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