#include "pch.hpp"            // includes ENTt from pch
#include "global.hpp"         // brings in event_dispatcher & registry

#include "TriggerSystem.hpp"
#include "TriggerEvents.hpp"
#include "TransformComponent.h"
#include "TriggerBoxComponent.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/glm.hpp>

#include <unordered_set>

namespace EngineFramework
{
    // --- compute AABB from world matrix and half extents ---
    static void ComputeAABB(const glm::mat4& world,
        const glm::vec3& half,
        glm::vec3& outMin,
        glm::vec3& outMax)
    {
        glm::vec3 c = glm::vec3(world[3]);
        glm::vec3 x = glm::vec3(world[0]);
        glm::vec3 y = glm::vec3(world[1]);
        glm::vec3 z = glm::vec3(world[2]);

        glm::vec3 ext =
            glm::abs(x) * half.x +
            glm::abs(y) * half.y +
            glm::abs(z) * half.z;

        outMin = c - ext;
        outMax = c + ext;
    }

    static bool Overlap(const glm::vec3& aMin, const glm::vec3& aMax,
        const glm::vec3& bMin, const glm::vec3& bMax)
    {
        return (aMin.x <= bMax.x && aMax.x >= bMin.x) &&
            (aMin.y <= bMax.y && aMax.y >= bMin.y) &&
            (aMin.z <= bMax.z && aMax.z >= bMin.z);
    }

    void TriggerSystem::Update(entt::registry& reg)
    {
        auto triggers = reg.view<TransformComponent, TriggerBoxComponent>();

        triggers.each(
            [&](entt::entity triggerEnt,
                TransformComponent& tf,
                TriggerBoxComponent& box)
            {
                if (!box.enabled)
                    return;

                glm::mat4 local =
                    glm::translate(glm::mat4(1.0f), box.localPosition) *
                    glm::mat4_cast(glm::quat(glm::radians(box.localEulerDeg))) *
                    glm::scale(glm::mat4(1.0f), box.halfExtents * 2.0f);

                glm::mat4 world = tf.modelMatrix * local;

                glm::vec3 trigMin, trigMax;
                ComputeAABB(world, box.halfExtents, trigMin, trigMax);

                std::unordered_set<entt::entity> insideNow;

                auto others = reg.view<TransformComponent>();
                for (auto otherEnt : others)
                {
                    if (otherEnt == triggerEnt)
                        continue;

                    auto& otf = others.get<TransformComponent>(otherEnt);

                    glm::vec3 half(0.25f);

                    glm::vec3 oMin, oMax;
                    ComputeAABB(otf.modelMatrix, half, oMin, oMax);

                    if (Overlap(trigMin, trigMax, oMin, oMax))
                    {
                        insideNow.insert(otherEnt);

                        if (box.currentInside.find(otherEnt) == box.currentInside.end())
                        {
                            event_dispatcher.trigger(Event_OnTriggerEnter{ triggerEnt, otherEnt });
                        }

                        if (otf.modelMatrix != otf.prevModelMatrix) {
                            event_dispatcher.trigger(Event_OnTriggerStay{ triggerEnt, otherEnt });
                        }
                    }
                }

                for (auto old : box.currentInside)
                {
                    if (insideNow.find(old) == insideNow.end())
                    {
                        event_dispatcher.trigger(Event_OnTriggerExit{ triggerEnt, old });
                    }
                }

                box.currentInside = std::move(insideNow);
            }
        );
    }

}