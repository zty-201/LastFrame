-- Bedroom_EnterTrigger.lua
-- 1. Player enters -> activate girl + start crying
-- 2. If checkpoint has been reached and player leaves -> deactivate girl permanently
-- 3. After that, re-entering the bedroom will NOT reactivate her

local DEBUG = true
local PLAYER_ENTITY_ID = 381

local GIRL_ENTITY_ID = 251
local LAUGH_SLOT     = "GirlLaughing"
local TENSION_SLOT   = "Tension"

local playerInside = false

local function activate_girl()
    SHARED = SHARED or {}

    if SHARED.BEDROOM_GIRL_DONE == true then
        if DEBUG then
            print("[Bedroom_EnterTrigger] activate_girl skipped, girl already permanently gone")
        end
        return
    end

    SHARED.BEDROOM_GIRL_ACTIVE = true

    Audio_SetSlotVolume(GIRL_ENTITY_ID, LAUGH_SLOT, 0.3)
    Audio_PlaySlot(GIRL_ENTITY_ID, LAUGH_SLOT)

    if DEBUG then
        print("[Bedroom_EnterTrigger] Activated bedroom girl")
    end
end

local function deactivate_girl_permanently()
    SHARED = SHARED or {}
    SHARED.BEDROOM_GIRL_ACTIVE = false
    SHARED.BEDROOM_GIRL_DONE   = true

    if type(Audio_StopSlot) == "function" then
        Audio_StopSlot(GIRL_ENTITY_ID, LAUGH_SLOT)
        Audio_StopSlot(GIRL_ENTITY_ID, TENSION_SLOT)
    end

    if DEBUG then
        print("[Bedroom_EnterTrigger] Bedroom girl permanently deactivated")
    end
end

function on_create(self)
    SHARED = SHARED or {}
    SHARED.BEDROOM_GIRL_ACTIVE = true
    SHARED.BEDROOM_CP_REACHED  = false
    SHARED.BEDROOM_GIRL_DONE   = false
    
    playerInside = false

    if DEBUG then
        print("[Bedroom_EnterTrigger] Ready on entity " .. tostring(EntityID))
    end
end

function on_trigger_enter(self, otherID)
    if otherID ~= PLAYER_ENTITY_ID then
        return
    end

    if playerInside then
        return
    end

    playerInside = true

    if SHARED and SHARED.BEDROOM_GIRL_DONE == true then
        if DEBUG then
            print("[Bedroom_EnterTrigger] Enter ignored, girl is permanently gone")
        end
        return
    end

    activate_girl()
end

function on_trigger_exit(self, otherID)
    if otherID ~= PLAYER_ENTITY_ID then
        return
    end

    playerInside = false

    if SHARED and SHARED.BEDROOM_CP_REACHED == true and SHARED.BEDROOM_GIRL_DONE ~= true then
        deactivate_girl_permanently()
    end
end

function on_destroy(self)
    playerInside = false
end