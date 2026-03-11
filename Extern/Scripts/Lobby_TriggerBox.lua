-- Lobby_ChairDragTrigger.lua
-- Attach to the Lobby_TriggerBox
-- When player enters, trigger two chairs to drag across the floor

local DEBUG = true
local PLAYER_ENTITY_ID = 5

local CHAIR_A_ID = 427
local CHAIR_B_ID = 428

local fired = false
local playerInsideThisZone = false

local function trigger_chair_drag()
    if fired then return end

    SHARED = SHARED or {}
    SHARED.CHAIR_DRAG_CMD = SHARED.CHAIR_DRAG_CMD or {}

    -- Chair A drags a shorter distance
    SHARED.CHAIR_DRAG_CMD[CHAIR_A_ID] = {
        cmd = "drag",
        by = EntityID
    }

    -- Chair B drags a longer distance
    SHARED.CHAIR_DRAG_CMD[CHAIR_B_ID] = {
        cmd = "drag",
        by = EntityID
    }

    fired = true

    if DEBUG then
        print("[Lobby_ChairDragTrigger] Triggered chair drag for chairs " ..
            tostring(CHAIR_A_ID) .. " and " .. tostring(CHAIR_B_ID))
    end
end

function on_create(self)
    SHARED = SHARED or {}
    SHARED.CHAIR_DRAG_CMD = SHARED.CHAIR_DRAG_CMD or {}

    fired = false
    playerInsideThisZone = false

    if DEBUG then
        print(string.format(
            "[Lobby_ChairDragTrigger] on_create | Zone=%d | ChairA=%d | ChairB=%d",
            EntityID, CHAIR_A_ID, CHAIR_B_ID
        ))
    end
end

function on_trigger_enter(self, otherID)
    if DEBUG then
        print(string.format(
            "[Lobby_ChairDragTrigger] Trigger ENTER | Zone=%d | Other=%d",
            EntityID, otherID
        ))
    end

    if otherID ~= PLAYER_ENTITY_ID then
        return
    end

    if playerInsideThisZone then
        return
    end
    playerInsideThisZone = true

    trigger_chair_drag()
end

function on_trigger_exit(self, otherID)
    if otherID ~= PLAYER_ENTITY_ID then
        return
    end

    playerInsideThisZone = false
end

function on_destroy(self)
    playerInsideThisZone = false
end