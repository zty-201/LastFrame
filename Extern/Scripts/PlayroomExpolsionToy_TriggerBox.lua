----------------------------------------------------------------
-- PlayroomToy_TriggerBox.lua
-- Attach to PLAYROOM TOY trigger entity (the interactable id InteractionBox detects)
--
-- Publishes:
--   SHARED.PLAYROOM_TOY_ICON_SHOW[TOY_ID] = true/false
--
-- Depends on InteractionBox:
--   SHARED.INTERACT_IN_ZONE[TOY_ID] = true/false
----------------------------------------------------------------

local DEBUG = false

-- If this script is on the trigger entity itself, leave 0.
-- Otherwise set to the interactable trigger id.
local TOY_ID_OVERRIDE = 0

local OUT_TABLE  = "PLAYROOM_TOY_ICON_SHOW"
local HELD_TABLE = "PLAYROOM_TOY_HELD"   -- optional

local function toy_id()
    return (TOY_ID_OVERRIDE ~= 0) and TOY_ID_OVERRIDE or EntityID
end

local function in_range(id)
    return SHARED
        and SHARED.INTERACT_IN_ZONE
        and SHARED.INTERACT_IN_ZONE[id] == true
end

local function is_held(id)
    return SHARED
        and SHARED[HELD_TABLE]
        and SHARED[HELD_TABLE][id] == true
end

function on_create(self)
    SHARED = SHARED or {}
    SHARED.INTERACT_IN_ZONE = SHARED.INTERACT_IN_ZONE or {}
    SHARED[OUT_TABLE] = SHARED[OUT_TABLE] or {}
    SHARED[HELD_TABLE] = SHARED[HELD_TABLE] or {}

    local id = toy_id()
    SHARED[OUT_TABLE][id] = false

    if DEBUG then
        print("[PlayroomToy_TriggerBox] create id=", id)
    end
end

function on_update(self, dt)
    local id = toy_id()

    local show = in_range(id) and (not is_held(id))
    SHARED[OUT_TABLE][id] = show
end
