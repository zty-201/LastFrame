-- Lobby_ChairDragTrigger.lua
-- Attach to the Lobby_TriggerBox
-- When player enters, trigger two chairs to drag across the floor

local DEBUG = true
local PLAYER_ENTITY_ID = 381

local CHAIR_A_ID = 427
local CHAIR_B_ID = 428

local fired = false
local playerInsideThisZone = false

local VOICEOVER_SFX_ID = "VoiceOver"
local VO_DELAY = 1.5

local pendingVO = false
local voTimer = 0.0

local function trigger_chair_drag()
    if fired then return end

    SHARED = SHARED or {}
    SHARED.CHAIR_DRAG_CMD = SHARED.CHAIR_DRAG_CMD or {}

    SHARED.CHAIR_DRAG_CMD[CHAIR_A_ID] = {
        cmd = "drag",
        by = EntityID
    }

    SHARED.CHAIR_DRAG_CMD[CHAIR_B_ID] = {
        cmd = "drag",
        by = EntityID
    }

    -- Start delayed VO
    pendingVO = true
    voTimer = 0.0

    fired = true

    if DEBUG then
        print("[Lobby_ChairDragTrigger] Triggered chair drag + VO scheduled")
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
    local otherNum = tonumber(otherID)
    local playerNum = tonumber(PLAYER_ENTITY_ID)

    if DEBUG then
        print(string.format(
            "[Lobby_ChairDragTrigger] Trigger ENTER | Zone=%d | Other=%s",
            EntityID, tostring(otherID)
        ))
    end

    if otherNum == nil or playerNum == nil or otherNum ~= playerNum then
        return
    end

    if playerInsideThisZone then
        return
    end
    playerInsideThisZone = true

    SHARED = SHARED or {}
    SHARED.CHAIR_DRAG_CMD = SHARED.CHAIR_DRAG_CMD or {}

    trigger_chair_drag()
end

function on_trigger_exit(self, otherID)
    local otherNum = tonumber(otherID)
    local playerNum = tonumber(PLAYER_ENTITY_ID)

    if otherNum == nil or playerNum == nil or otherNum ~= playerNum then
        return
    end

    playerInsideThisZone = false
end

function on_update(self, dt)
    dt = dt or 0.016

    if pendingVO then
        voTimer = voTimer + dt
        if voTimer >= VO_DELAY then
            pendingVO = false
            voTimer = 0.0

            if type(Audio_PlaySlot) == "function" then
                Audio_PlaySlot(EntityID, VOICEOVER_SFX_ID)
            end

            if DEBUG then
                print("[Lobby_ChairDragTrigger] VoiceOver played after delay")
            end
        end
    end
end

function on_destroy(self)
    playerInsideThisZone = false
end