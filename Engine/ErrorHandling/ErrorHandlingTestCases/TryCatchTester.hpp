#pragma once
#include "ISystem.hpp"
#include <stdexcept>

class FailingSystem : public EngineFramework::ISystem {
public:
    void Initialize() override {}
    void Update(float) override { throw std::runtime_error("Update crashed on purpose!"); }
    void PostUpdate(float) override {}
};
