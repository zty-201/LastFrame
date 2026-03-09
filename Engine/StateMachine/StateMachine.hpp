#pragma once

#include "IState.hpp"
#include "StateParameters.hpp"
#include "Transition.hpp"

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace Engine
{
    class StateMachine
    {
    public:
        StateMachine() = default;
        ~StateMachine() = default;

        StateMachine(const StateMachine&) = delete;
        StateMachine& operator=(const StateMachine&) = delete;

        bool AddState(std::unique_ptr<IState> state);
        bool HasState(const std::string& stateName) const;
        IState* GetState(const std::string& stateName);
        const IState* GetState(const std::string& stateName) const;

        void AddTransition(const Transition& transition);
        void ClearTransitions();

        void AddBoolTransition(const std::string& from,
            const std::string& to,
            const std::string& parameterName,
            bool expectedValue = true,
            bool evaluateBeforeUpdate = false);

        void AddTriggerTransition(const std::string& from,
            const std::string& to,
            const std::string& triggerName,
            bool evaluateBeforeUpdate = false);

        void AddCustomTransition(const std::string& from,
            const std::string& to,
            std::function<bool()> customCondition,
            bool evaluateBeforeUpdate = false);

        bool SetInitialState(const std::string& stateName);
        bool ChangeState(const std::string& stateName);

        void Update(float dt);
        void Render();

        IState* GetCurrentState();
        const IState* GetCurrentState() const;

        const std::string& GetCurrentStateName() const;
        bool IsInState(const std::string& stateName) const;

        void SetBool(const std::string& name, bool value);
        bool GetBool(const std::string& name, bool defaultValue = false) const;
        bool HasBool(const std::string& name) const;

        void SetInt(const std::string& name, int value);
        int GetInt(const std::string& name, int defaultValue = 0) const;
        bool HasInt(const std::string& name) const;

        void SetFloat(const std::string& name, float value);
        float GetFloat(const std::string& name, float defaultValue = 0.0f) const;
        bool HasFloat(const std::string& name) const;

        void SetTrigger(const std::string& name);
        bool HasTrigger(const std::string& name) const;
        bool ConsumeTrigger(const std::string& name);
        void ResetTrigger(const std::string& name);

        StateParameters& GetParameters();
        const StateParameters& GetParameters() const;
        
        bool HasCurrentState() const;
        void Reset();

    private:
        bool EvaluateTransitions(bool evaluateBeforeUpdate);
        bool EvaluateTransition(const Transition& transition);
        bool EvaluateTransitionConditions(const Transition& transition);
        bool TransitionMatchesCurrentState(const Transition& transition) const;

    private:
        std::unordered_map<std::string, std::unique_ptr<IState>> m_states;
        std::vector<Transition> m_transitions;
        StateParameters m_parameters;

        IState* m_currentState = nullptr;
        std::string m_currentStateName;
        bool m_isChangingState = false;
    };
}