#pragma once
#include <entt/entity/entity.hpp>

struct Event_OnTriggerEnter {
    entt::entity trigger;
    entt::entity other;
};

struct Event_OnTriggerStay {
    entt::entity trigger;
    entt::entity other;
};

struct Event_OnTriggerExit {
    entt::entity trigger;
    entt::entity other;
};
