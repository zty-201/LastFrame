#include "pch.hpp"
#include "SplashState.hpp"

const std::string& SplashState::GetName() const
{
    return m_name;
}

void SplashState::OnEnter(Engine::StateMachine& machine)
{
    m_timer = 0.0f;
    machine.SetBool("SplashFinished", false);
    machine.ResetTrigger("SkipSplash");
}

void SplashState::OnExit(Engine::StateMachine& machine)
{
    UNREFERENCED_PARAMETER(machine);
}

void SplashState::Update(Engine::StateMachine& machine, float dt)
{
    m_timer += dt;

    if (m_timer >= m_duration)
    {
        machine.SetBool("SplashFinished", true);
    }
}

void SplashState::Render(Engine::StateMachine& machine)
{
    UNREFERENCED_PARAMETER(machine);
}