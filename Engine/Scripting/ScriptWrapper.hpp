#ifndef SCRIPT_WRAPPER_HPP
#define SCRIPT_WRAPPER_HPP

#include "ISystem.hpp"
#include "Script.hpp"

namespace EngineFramework
{
    class ScriptWrapper : public ISystem
    {
    public:
        ScriptWrapper();
        ~ScriptWrapper();

        // Optional: pseudo-singleton getter (mirrors your AnimationSystem style)
        static ScriptWrapper& GetInstance() { return *ScriptWrapperPtr; }

    private:
        // ISystem implementation
        void Initialize() override;
        void Update(float delta_time) override;
        void PostUpdate(float delta_time) override { UNREFERENCED_PARAMETER(delta_time); }

    private:
        static inline ScriptWrapper* ScriptWrapperPtr = nullptr;

        bool m_Initialized = false;
        bool m_RuntimeStarted = false;
    };
}

#endif // SCRIPT_WRAPPER_HPP
