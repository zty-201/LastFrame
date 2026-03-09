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
    ui.StartSplashSequence();
}

void SplashState::OnExit(Engine::StateMachine& machine)
{
    UNREFERENCED_PARAMETER(machine);

    auto& ui = EngineFramework::UI_System::GetInstance();
    ui.StopCutsceneSequence(true);

    LOGI("[State] Exit Splash");
}

void SplashState::Update(Engine::StateMachine& machine, float dt)
{
    UNREFERENCED_PARAMETER(dt);

    auto& ui = EngineFramework::UI_System::GetInstance();

    if (!ui.IsCutscenePlaying())
    {
        LOGI("[State] Splash finished -> MainMenu");
        machine.SetBool("SplashFinished", true);
    }

    // Optional skip support later:
    // if (InputManager::GetInstance().GetAction("JUMP"))
    // {
    //     machine.SetTrigger("SkipSplash");
    // }
}

void SplashState::Render(Engine::StateMachine& machine)
{
    UNREFERENCED_PARAMETER(machine);
}