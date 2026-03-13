-- Roof_TriggerBox.lua
-- Plays "Scary" once, the first time the player enters the trigger box

local DEBUG = true
local PLAYER_ENTITY_ID = 381
local SCARY_AUDIO_ID = "Scary"

local fired = false
local playerInsideThisZone = false

local function play_scary_once()
    if fired then return end

    if type(Audio_PlaySlot) == "function" then
        Audio_PlaySlot(EntityID, SCARY_AUDIO_ID)
    end

    fired = true

    if DEBUG then
        print("[Roof_TriggerBox] Played Scary audio once")
    end
end

function on_create(self)
    fired = false
    playerInsideThisZone = false

    if DEBUG then
        print(string.format(
            "[Roof_TriggerBox] on_create | Zone=%d",
            EntityID
        ))
    end
end

function on_trigger_enter(self, otherID)
    if DEBUG then
        print(string.format(
            "[Roof_TriggerBox] Trigger ENTER | Zone=%d | Other=%d",
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

    play_scary_once()
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