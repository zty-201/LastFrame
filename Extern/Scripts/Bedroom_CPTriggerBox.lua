-- Bedroom_CPTriggerBox.lua
-- When player reaches this deeper trigger, mark bedroom checkpoint as reached

local DEBUG = true
local PLAYER_ENTITY_ID = 381

function on_create(self)
    SHARED = SHARED or {}
    SHARED.BEDROOM_CP_REACHED = SHARED.BEDROOM_CP_REACHED or false

    if DEBUG then
        print("[Bedroom_CPTriggerBox] Ready on entity " .. tostring(EntityID))
    end
end

function on_trigger_enter(self, otherID)
    if otherID ~= PLAYER_ENTITY_ID then
        return
    end

    SHARED = SHARED or {}
    SHARED.BEDROOM_CP_REACHED = true

    if DEBUG then
        print("[Bedroom_CPTriggerBox] Checkpoint reached")
    end
end

function on_trigger_exit(self, otherID)
end

function on_destroy(self)
end