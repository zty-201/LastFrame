----------------------------------------------------------------
-- SFX_LightningEmitter.lua
----------------------------------------------------------------

print("[LightningEmitter] Script loaded")

local DEBUG = true

local LIGHTNING_SLOT_ID = "SoftLightning"

local LIGHTNING_TABLE = "LIGHTNING_CMD"
local ROOFTOP_KEY = "ROOFTOP_LIGHTNING"

local lastProcessedEventId = -1
local myEntityID = -1

local function play_3d_sfx(slotId)
    if slotId == nil then
        return false
    end

    if type(Audio_StopSlot) == "function" then
        Audio_StopSlot(myEntityID, slotId)
    end

    if type(Audio_PlaySlot) == "function" then
        Audio_PlaySlot(myEntityID, slotId)
        return true
    end

    return false
end

function on_create(self)
    myEntityID = EntityID

    SHARED = SHARED or {}
    SHARED[LIGHTNING_TABLE] = SHARED[LIGHTNING_TABLE] or {}

    lastProcessedEventId = -1

    print("[RooftopSoftLightning] on_create EntityID = " .. tostring(myEntityID))
end

function on_update(self, dt)
    SHARED = SHARED or {}
    SHARED[LIGHTNING_TABLE] = SHARED[LIGHTNING_TABLE] or {}

    local lightningTable = SHARED[LIGHTNING_TABLE]
    local lightningData = lightningTable[ROOFTOP_KEY]

    if lightningData ~= nil and lightningData.eventId ~= lastProcessedEventId then
        lastProcessedEventId = lightningData.eventId

        print("[RooftopSoftLightning] EntityID = " .. tostring(myEntityID) ..
              " received eventId = " .. tostring(lightningData.eventId))

        play_3d_sfx(LIGHTNING_SLOT_ID)

        print("[RooftopSoftLightning] EntityID = " .. tostring(myEntityID) ..
              " played sound for eventId = " .. tostring(lightningData.eventId))
    end
end

function on_destroy(self)
    print("[RooftopSoftLightning] destroyed EntityID = " .. tostring(myEntityID))
end