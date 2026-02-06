----------------------------------------------------------------
-- TriggerZone_Laughter_OnEnterStopOnExit.lua
-- When player enters INTERACT_IN_ZONE[TARGET_ID], play "Laughter".
-- When player leaves, stop "Laughter".
-- Re-enter -> plays again.
----------------------------------------------------------------

local DEBUG = true

local TARGET_ID = 643         -- the interactable/trigger id that appears in INTERACT_IN_ZONE
local SLOT_ID   = "Laughter"  -- Slot ID in AudioComponent (from your screenshot)

local wasInZone = false

local function in_zone()
    return SHARED
        and SHARED.INTERACT_IN_ZONE
        and SHARED.INTERACT_IN_ZONE[TARGET_ID] == true
end

local function play()
    if type(Audio_PlaySlot) == "function" then
        Audio_PlaySlot(EntityID, SLOT_ID)
        if DEBUG then
            print("[ZoneLaughter] PLAY slot=" .. tostring(SLOT_ID) .. " from entity=" .. tostring(EntityID))
        end
    else
        print("[ZoneLaughter] ERROR: Audio_PlaySlot not available")
    end
end

local function stop()
    -- Your engine likely has Audio_StopSlot(entityId, slotId)
    if type(Audio_StopSlot) == "function" then
        Audio_StopSlot(EntityID, SLOT_ID)
        if DEBUG then
            print("[ZoneLaughter] STOP slot=" .. tostring(SLOT_ID) .. " from entity=" .. tostring(EntityID))
        end
        return
    end

    -- Fallback: if you only have stop-all for this entity
    if type(Audio_StopAllSlots) == "function" then
        Audio_StopAllSlots(EntityID)
        if DEBUG then
            print("[ZoneLaughter] STOP ALL (fallback) entity=" .. tostring(EntityID))
        end
        return
    end

    print("[ZoneLaughter] WARN: No Audio_StopSlot / Audio_StopAllSlots available")
end

function on_create(self)
    SHARED.INTERACT_IN_ZONE = SHARED.INTERACT_IN_ZONE or {}
    wasInZone = false

    if DEBUG then
        print("[ZoneLaughter] ready. TARGET_ID=" .. tostring(TARGET_ID) .. " SLOT_ID=" .. tostring(SLOT_ID))
    end
end

function on_update(self, dt)
    local nowInZone = in_zone()

    -- ENTER edge
    if nowInZone and (not wasInZone) then
        play()
    end

    -- EXIT edge
    if (not nowInZone) and wasInZone then
        stop()
    end

    wasInZone = nowInZone
end
