#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>

namespace Engine
{
    class StateParameters
    {
    public:
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

        void ClearAll();

    private:
        std::unordered_map<std::string, bool> m_boolParams;
        std::unordered_map<std::string, int> m_intParams;
        std::unordered_map<std::string, float> m_floatParams;
        std::unordered_set<std::string> m_triggers;
    };
}