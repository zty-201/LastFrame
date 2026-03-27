#include "StateMachine.hpp"

namespace Engine
{
    void StateParameters::SetBool(const std::string& name, bool value)
    {
        m_boolParams[name] = value;
    }

    bool StateParameters::GetBool(const std::string& name, bool defaultValue) const
    {
        auto it = m_boolParams.find(name);
        return (it != m_boolParams.end()) ? it->second : defaultValue;
    }

    bool StateParameters::HasBool(const std::string& name) const
    {
        return m_boolParams.find(name) != m_boolParams.end();
    }

    void StateParameters::SetInt(const std::string& name, int value)
    {
        m_intParams[name] = value;
    }

    int StateParameters::GetInt(const std::string& name, int defaultValue) const
    {
        auto it = m_intParams.find(name);
        return (it != m_intParams.end()) ? it->second : defaultValue;
    }

    bool StateParameters::HasInt(const std::string& name) const
    {
        return m_intParams.find(name) != m_intParams.end();
    }

    void StateParameters::SetFloat(const std::string& name, float value)
    {
        m_floatParams[name] = value;
    }

    float StateParameters::GetFloat(const std::string& name, float defaultValue) const
    {
        auto it = m_floatParams.find(name);
        return (it != m_floatParams.end()) ? it->second : defaultValue;
    }

    bool StateParameters::HasFloat(const std::string& name) const
    {
        return m_floatParams.find(name) != m_floatParams.end();
    }

    void StateParameters::SetTrigger(const std::string& name)
    {
        m_triggers.insert(name);
    }

    bool StateParameters::HasTrigger(const std::string& name) const
    {
        return m_triggers.find(name) != m_triggers.end();
    }

    bool StateParameters::ConsumeTrigger(const std::string& name)
    {
        auto it = m_triggers.find(name);
        if (it == m_triggers.end())
        {
            return false;
        }

        m_triggers.erase(it);
        return true;
    }

    void StateParameters::ResetTrigger(const std::string& name)
    {
        m_triggers.erase(name);
    }

    void StateParameters::ClearAll()
    {
        m_boolParams.clear();
        m_intParams.clear();
        m_floatParams.clear();
        m_triggers.clear();
    }

    bool StateMachine::AddState(std::unique_ptr<IState> state)
    {
        if (!state)
        {
            return false;
        }

        const std::string& stateName = state->GetName();
        if (stateName.empty())
        {
            return false;
        }

        if (m_states.find(stateName) != m_states.end())
        {
            return false;
        }

        m_states.emplace(stateName, std::move(state));
        return true;
    }

    bool StateMachine::HasState(const std::string& stateName) const
    {
        return m_states.find(stateName) != m_states.end();
    }

    IState* StateMachine::GetState(const std::string& stateName)
    {
        auto it = m_states.find(stateName);
        return (it != m_states.end()) ? it->second.get() : nullptr;
    }

    const IState* StateMachine::GetState(const std::string& stateName) const
    {
        auto it = m_states.find(stateName);
        return (it != m_states.end()) ? it->second.get() : nullptr;
    }

    void StateMachine::AddTransition(const Transition& transition)
    {
        m_transitions.push_back(transition);
    }

    void StateMachine::ClearTransitions()
    {
        m_transitions.clear();
    }

    void StateMachine::AddBoolTransition(const std::string& from,
        const std::string& to,
        const std::string& parameterName,
        bool expectedValue,
        bool evaluateBeforeUpdate)
    {
        Transition transition;
        transition.fromState = from;
        transition.toState = to;
        transition.evaluateBeforeUpdate = evaluateBeforeUpdate;
        transition.requireAllConditions = true;

        TransitionCondition condition;
        condition.type = TransitionConditionType::BoolEquals;
        condition.parameterName = parameterName;
        condition.expectedBoolValue = expectedValue;

        transition.conditions.push_back(condition);
        AddTransition(transition);
    }

    void StateMachine::AddTriggerTransition(const std::string& from,
        const std::string& to,
        const std::string& triggerName,
        bool evaluateBeforeUpdate)
    {
        Transition transition;
        transition.fromState = from;
        transition.toState = to;
        transition.evaluateBeforeUpdate = evaluateBeforeUpdate;
        transition.requireAllConditions = true;

        TransitionCondition condition;
        condition.type = TransitionConditionType::Triggered;
        condition.parameterName = triggerName;

        transition.conditions.push_back(condition);
        AddTransition(transition);
    }

    void StateMachine::AddCustomTransition(const std::string& from,
        const std::string& to,
        std::function<bool()> customCondition,
        bool evaluateBeforeUpdate)
    {
        Transition transition;
        transition.fromState = from;
        transition.toState = to;
        transition.evaluateBeforeUpdate = evaluateBeforeUpdate;
        transition.customCondition = std::move(customCondition);
        AddTransition(transition);
    }

    bool StateMachine::SetInitialState(const std::string& stateName)
    {
        IState* state = GetState(stateName);
        if (!state)
        {
            LOGE("[StateMachine] Failed to set initial state: %s", stateName.c_str());
            return false;
        }

        m_currentState = state;
        m_currentStateName = stateName;

        LOGI("[StateMachine] Initial state set to: %s", stateName.c_str());

        m_currentState->OnEnter(*this);
        return true;
    }

    bool StateMachine::ChangeState(const std::string& stateName)
    {
        if (m_isChangingState)
        {
            LOGE("[StateMachine] Re-entrant ChangeState blocked: %s", stateName.c_str());
            return false;
        }

        if (stateName.empty())
        {
            LOGE("[StateMachine] ChangeState failed: empty target state");
            return false;
        }

        IState* newState = GetState(stateName);
        if (!newState)
        {
            LOGE("[StateMachine] ChangeState failed: target state not found: %s", stateName.c_str());
            return false;
        }

        if (m_currentState == newState)
        {
            LOGI("[StateMachine] Already in state: %s", stateName.c_str());
            return true;
        }

        LOGI("[StateMachine] Transition: %s -> %s",
            m_currentStateName.c_str(),
            stateName.c_str());

        m_isChangingState = true;

        if (m_currentState)
        {
            m_currentState->OnExit(*this);
        }

        m_currentState = newState;
        m_currentStateName = stateName;

        m_currentState->OnEnter(*this);

        m_isChangingState = false;
        return true;
    }

    void StateMachine::Update(float dt)
    {
        if (!m_currentState)
        {
            return;
        }


        //if (m_currentState->GetName() == "Pause" /*&& !triggered_already*/) {
        //    //trigger event

        //    /*Pause_Event pause_event{};
        //    pause_event.is_pause = true;
        //    event_dispatcher.trigger<Pause_Event>(std::move(pause_event));
        //    */
        //    std::cout << "hello, i already triggered this bitch =================================\n";
        //}


        if (EvaluateTransitions(true))
        {
            if (!m_currentState)
            {
                return;
            }
        }

        m_currentState->Update(*this, dt);
        EvaluateTransitions(false);
    }

    void StateMachine::Render()
    {
        if (m_currentState)
        {
            m_currentState->Render(*this);
        }
    }

    IState* StateMachine::GetCurrentState()
    {
        return m_currentState;
    }

    const IState* StateMachine::GetCurrentState() const
    {
        return m_currentState;
    }

    const std::string& StateMachine::GetCurrentStateName() const
    {
        return m_currentStateName;
    }

    bool StateMachine::IsInState(const std::string& stateName) const
    {
        return m_currentStateName == stateName;
    }

    void StateMachine::SetBool(const std::string& name, bool value)
    {
        m_parameters.SetBool(name, value);
    }

    bool StateMachine::GetBool(const std::string& name, bool defaultValue) const
    {
        return m_parameters.GetBool(name, defaultValue);
    }

    bool StateMachine::HasBool(const std::string& name) const
    {
        return m_parameters.HasBool(name);
    }

    void StateMachine::SetInt(const std::string& name, int value)
    {
        m_parameters.SetInt(name, value);
    }

    int StateMachine::GetInt(const std::string& name, int defaultValue) const
    {
        return m_parameters.GetInt(name, defaultValue);
    }

    bool StateMachine::HasInt(const std::string& name) const
    {
        return m_parameters.HasInt(name);
    }

    void StateMachine::SetFloat(const std::string& name, float value)
    {
        m_parameters.SetFloat(name, value);
    }

    float StateMachine::GetFloat(const std::string& name, float defaultValue) const
    {
        return m_parameters.GetFloat(name, defaultValue);
    }

    bool StateMachine::HasFloat(const std::string& name) const
    {
        return m_parameters.HasFloat(name);
    }

    void StateMachine::SetTrigger(const std::string& name)
    {
        m_parameters.SetTrigger(name);
    }

    bool StateMachine::HasTrigger(const std::string& name) const
    {
        return m_parameters.HasTrigger(name);
    }

    bool StateMachine::ConsumeTrigger(const std::string& name)
    {
        return m_parameters.ConsumeTrigger(name);
    }

    void StateMachine::ResetTrigger(const std::string& name)
    {
        m_parameters.ResetTrigger(name);
    }

    StateParameters& StateMachine::GetParameters()
    {
        return m_parameters;
    }

    const StateParameters& StateMachine::GetParameters() const
    {
        return m_parameters;
    }

    bool StateMachine::HasCurrentState() const
    {
        return m_currentState != nullptr;
    }

    void StateMachine::Reset()
    {
        if (m_currentState)
        {
            m_currentState->OnExit(*this);
        }

        m_currentState = nullptr;
        m_currentStateName.clear();
        m_parameters.ClearAll();
        m_transitions.clear();
        m_states.clear();
        m_isChangingState = false;
    }

    bool StateMachine::EvaluateTransitions(bool evaluateBeforeUpdate)
    {
        if (!m_currentState)
        {
            return false;
        }



        for (const Transition& transition : m_transitions)
        {
            if (transition.evaluateBeforeUpdate != evaluateBeforeUpdate)
            {
                continue;
            }

            if (!TransitionMatchesCurrentState(transition))
            {
                continue;
            }

            if (!EvaluateTransition(transition))
            {
                continue;
            }

            return ChangeState(transition.toState);
        }

        return false;
    }

    bool StateMachine::EvaluateTransition(const Transition& transition)
    {
        bool parameterConditionsPassed = EvaluateTransitionConditions(transition);

        if (!parameterConditionsPassed)
        {
            return false;
        }

        if (transition.customCondition)
        {
            return transition.customCondition();
        }

        if (!transition.conditions.empty())
        {
            return true;
        }

        return false;
    }

    bool StateMachine::EvaluateTransitionConditions(const Transition& transition)
    {
        if (transition.conditions.empty())
        {
            return true;
        }

        bool anyPassed = false;

        for (const TransitionCondition& condition : transition.conditions)
        {
            bool passed = false;

            switch (condition.type)
            {
            case TransitionConditionType::BoolEquals:
                passed = (m_parameters.GetBool(condition.parameterName) == condition.expectedBoolValue);
                break;

            case TransitionConditionType::Triggered:
                passed = m_parameters.HasTrigger(condition.parameterName);
                break;

            case TransitionConditionType::None:
            default:
                passed = false;
                break;
            }

            if (transition.requireAllConditions)
            {
                if (!passed)
                {
                    return false;
                }
            }
            else
            {
                if (passed)
                {
                    anyPassed = true;
                }
            }
        }

        if (!transition.requireAllConditions && !anyPassed)
        {
            return false;
        }

        for (const TransitionCondition& condition : transition.conditions)
        {
            if (condition.type == TransitionConditionType::Triggered)
            {
                m_parameters.ConsumeTrigger(condition.parameterName);
            }
        }

        return true;
    }

    bool StateMachine::TransitionMatchesCurrentState(const Transition& transition) const
    {
        return transition.fromState == m_currentStateName;
    }
}