#include "pch.hpp"
#include "SplashState.hpp"
#include "UISystem.hpp"

const std::string& SplashState::GetName() const
{
    return m_name;
}

void SplashState::OnEnter(Engine::StateMachine& machine)
{
    LOGI("[State] Enter Splash");

    m_timer = 0.0f;
    machine.SetBool("SplashFinished", false);
    machine.ResetTrigger("SkipSplash");

    auto& ui = EngineFramework::UI_System::GetInstance();
    ui.closeAllMenu();
    ui.displayMenu("SplashMenu");
}

void SplashState::OnExit(Engine::StateMachine& machine)
{
    UNREFERENCED_PARAMETER(machine);

    EngineFramework::UI_System::GetInstance().closeAllMenu();
    LOGI("[State] Exit Splash");
}

void SplashState::Update(Engine::StateMachine& machine, float dt)
{
    m_timer += dt;

    if (m_timer >= m_duration)
    {
        LOGI("[State] Splash finished -> MainMenu");
        machine.SetBool("SplashFinished", true);
    }

    // Optional skip
    // if (Input pressed)
    // {
    //     machine.SetTrigger("SkipSplash");
    // }
}

void SplashState::Render(Engine::StateMachine& machine)
{
    UNREFERENCED_PARAMETER(machine);
}