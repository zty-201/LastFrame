#ifndef ANIMATION_SYSTEM_HPP
#define ANIMATION_SYSTEM_HPP

#include "ISystem.hpp"
#include "scene.h"

namespace EngineFramework
{
    class AnimationSystem : public ISystem
    {
    public:
        AnimationSystem();
        ~AnimationSystem();

        // ISystem implementation
        void Initialize() override {};
        void Update(float deltaTime) override;

    private:
		void ComputeWorldMatrix(const TransformComponent& transform, glm::mat4& outWorldMatrix);
		static inline AnimationSystem* AnimationSystemPtr = nullptr;
    };
}

#endif // GRAPHICS_SYSTEM_HPP