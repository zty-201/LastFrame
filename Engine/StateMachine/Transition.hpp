#pragma once

#include <functional>
#include <string>
#include <vector>

namespace Engine
{
    enum class TransitionConditionType
    {
        None = 0,
        BoolEquals,
        Triggered
    };

    struct TransitionCondition
    {
        TransitionConditionType type = TransitionConditionType::None;
        std::string parameterName;
        bool expectedBoolValue = true;
    };

    struct Transition
    {
        std::string fromState;
        std::string toState;

        std::vector<TransitionCondition> conditions;
        std::function<bool()> customCondition = nullptr;

        bool requireAllConditions = true;
        bool evaluateBeforeUpdate = false;
    };
}