----------------------------------------------------------------
-- CanteenLightOffOnTrigger.lua
-- Attach to CANTEEN lights only
-- Turns off this light shortly after trigger fires
-- Turns it back on when toy is put down
----------------------------------------------------------------

local DEBUG = false

local LIGHTS_TABLE = "LIGHTS_CMD"
local EVENT_OFF_KEY = "CANTEEN_LIGHTS_OFF"
local EVENT_ON_KEY = "CANTEEN_LIGHTS_ON"
local DISABLE_KEY = "CANTEEN_LIGHTS_DISABLED"

local LIGHT_BLOWOUT_SFX_ID = "LightBlowOut"
local LIGHT_ON_SFX_ID = "LightOn"

local SHUTDOWN_DELAY = 1.2
local shutdownTimer = 0.0
local waitingForShutdown = false

local originalIntensity = 1.0
local isCurrentlyOff = false

local function play_3d_sfx(slotId)
    if type(Audio_PlaySlot) == "function" and slotId ~= nil then
        Audio_PlaySlot(EntityID, slotId)
        return true
    end
    return false
end

function on_create(self)
    SHARED = SHARED or {}
    SHARED[LIGHTS_TABLE] = SHARED[LIGHTS_TABLE] or {}

    shutdownTimer = 0.0
    waitingForShutdown = false
    isCurrentlyOff = false

    -- Safe defaults on startup
    if SHARED[LIGHTS_TABLE][EVENT_OFF_KEY] == nil then
        SHARED[LIGHTS_TABLE][EVENT_OFF_KEY] = false
    end
    if SHARED[LIGHTS_TABLE][EVENT_ON_KEY] == nil then
        SHARED[LIGHTS_TABLE][EVENT_ON_KEY] = false
    end
    if SHARED[LIGHTS_TABLE][DISABLE_KEY] == nil then
        SHARED[LIGHTS_TABLE][DISABLE_KEY] = false
    end

    if EntityLight == nil then
        print("[CanteenLightOffOnTrigger] ERROR: EntityLight missing on entity " .. tostring(EntityID))
        return
    end

    originalIntensity = EntityLight:GetIntensity()

    if DEBUG then
        print(string.format("[CanteenLightOffOnTrigger] Ready | Entity=%d", EntityID))
    end
end

function on_update(self, dt)
    if EntityLight == nil then return end
    dt = dt or 0.016

    local t = SHARED and SHARED[LIGHTS_TABLE]
    if t == nil then return end

    ----------------------------------------------------------------
    -- LIGHTS ON REQUEST
    ----------------------------------------------------------------
    if t[EVENT_ON_KEY] == true then
        if isCurrentlyOff then
            EntityLight:SetIntensity(originalIntensity)
            play_3d_sfx(LIGHT_ON_SFX_ID)

            isCurrentlyOff = false
            waitingForShutdown = false
            shutdownTimer = 0.0
            t[DISABLE_KEY] = false

            if DEBUG then
                print(string.format("[CanteenLightOffOnTrigger] Light ON | Entity=%d", EntityID))
            end
        end

        return
    end

    ----------------------------------------------------------------
    -- LIGHTS OFF REQUEST
    ----------------------------------------------------------------
    if t[EVENT_OFF_KEY] == true then
        if not waitingForShutdown and not isCurrentlyOff then
            waitingForShutdown = true
            shutdownTimer = 0.0

            if DEBUG then
                print("[CanteenLightOffOnTrigger] Shutdown timer started")
            end
        end
    end

    if waitingForShutdown then
        shutdownTimer = shutdownTimer + dt

        if shutdownTimer >= SHUTDOWN_DELAY then
            EntityLight:SetIntensity(0.0)
            play_3d_sfx(LIGHT_BLOWOUT_SFX_ID)

            isCurrentlyOff = true
            waitingForShutdown = false
            shutdownTimer = 0.0
            t[DISABLE_KEY] = true

            if DEBUG then
                print(string.format("[CanteenLightOffOnTrigger] Light OFF | Entity=%d", EntityID))
            end
        end
    end
end