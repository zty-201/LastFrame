----------------------------------------------------------------
-- Trigger_Audio_OnRange.lua
-- Plays audio when player enters InteractionBox range
-- Stops audio when player leaves range
----------------------------------------------------------------

local DEBUG = true
local AUDIO_SLOT_ID = "GirlLaughing"
local TARGET_ID = 688              -- interactable entity ID

local wasInRange = false

function on_create(self)
    SHARED.INTERACT_IN_ZONE = SHARED.INTERACT_IN_ZONE or {}

    if DEBUG then
        print("[Trigger_Audio] on_create for entity " .. tostring(EntityID))
    end
end

function on_update(self, dt)
    local inRange =
        SHARED.INTERACT_IN_ZONE
        and SHARED.INTERACT_IN_ZONE[TARGET_ID] == true

    -- ENTER
    if inRange and not wasInRange then
        Audio_PlaySlot(EntityID, AUDIO_SLOT_ID)
        if DEBUG then
            print("[Trigger_Audio] ENTER -> play", AUDIO_SLOT_ID)
        end
    end

    -- EXIT
    if not inRange and wasInRange then
        Audio_StopSlot(EntityID, AUDIO_SLOT_ID)
        if DEBUG then
            print("[Trigger_Audio] EXIT -> stop", AUDIO_SLOT_ID)
        end
    end

    wasInRange = inRange
end
