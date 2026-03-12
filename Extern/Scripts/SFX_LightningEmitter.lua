----------------------------------------------------------------
-- SFX_Rooftop_SoftLightning.lua
-- Plays soft lightning SFX whenever the spotlight sends
-- a rooftop lightning cue.
----------------------------------------------------------------

print("[RooftopSoftLightning] Script loaded")

local DEBUG = true

local LIGHTNING_SLOT_ID = "SoftLightning"

local LIGHTNING_TABLE = "LIGHTNING_CMD"
local ROOFTOP_KEY = "ROOFTOP_LIGHTNING"

local lastProcessedEventId = -1

local function play_3d_sfx(slotId)
    if slotId == nil then
        return false
    end

    if type(Audio_StopSlot) == "function" then
        Audio_StopSlot(EntityID, slotId)
    end

    if type(Audio_PlaySlot) == "function" then
        Audio_PlaySlot(EntityID, slotId)
        return true
    end

    return false
end

function on_create(self)
    SHARED = SHARED or {}
    SHARED[LIGHTNING_TABLE] = SHARED[LIGHTNING_TABLE] or {}

    lastProcessedEventId = -1

    if DEBUG then
        print("[RooftopSoftLightning] on_create EntityID =", EntityID)
    end
end

function on_update(self, dt)
    SHARED = SHARED or {}
    SHARED[LIGHTNING_TABLE] = SHARED[LIGHTNING_TABLE] or {}

    local lightningTable = SHARED[LIGHTNING_TABLE]
    local lightningData = lightningTable[ROOFTOP_KEY]

    if lightningData ~= nil and lightningData.eventId ~= lastProcessedEventId then
        lastProcessedEventId = lightningData.eventId
        play_3d_sfx(LIGHTNING_SLOT_ID)

        if DEBUG then
            print("[RooftopSoftLightning] Played soft lightning for eventId = " .. tostring(lightningData.eventId))
        end
    end
end

function on_destroy(self)
    if DEBUG then
        print("[RooftopSoftLightning] destroyed")
    end
end