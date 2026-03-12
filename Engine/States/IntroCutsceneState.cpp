#include "pch.hpp"
#include "IntroCutsceneState.hpp"
#include "UISystem.hpp"

const std::string& IntroCutsceneState::GetName() const
{
    return m_name;
}

void IntroCutsceneState::OnEnter(Engine::StateMachine& machine)
{
    LOGI("[State] Enter IntroCutscene");

    machine.SetBool("IntroCutsceneFinished", false);
    machine.ResetTrigger("SkipIntro");

    auto& ui = EngineFramework::UI_System::GetInstance();
    ui.closeAllMenu();

    ui.StartCutsceneSequence(
        { "Cutscene_1", "Cutscene_2", "Cutscene_3", "Cutscene_4", "Cutscene_5"},
        3.0f,
        true
    );
}

void IntroCutsceneState::OnExit(Engine::StateMachine& machine)
{
    UNREFERENCED_PARAMETER(machine);

    auto& ui = EngineFramework::UI_System::GetInstance();
    ui.StopCutsceneSequence(true);

    LOGI("[State] Exit IntroCutscene");
}

void IntroCutsceneState::Update(Engine::StateMachine& machine, float dt)
{
    UNREFERENCED_PARAMETER(dt);

    auto& ui = EngineFramework::UI_System::GetInstance();

    if (!ui.IsCutscenePlaying())
    {
        LOGI("[State] IntroCutscene finished -> Gameplay");
        machine.SetBool("IntroCutsceneFinished", true);
    }
}

void IntroCutsceneState::Render(Engine::StateMachine& machine)
{
    UNREFERENCED_PARAMETER(machine);
}