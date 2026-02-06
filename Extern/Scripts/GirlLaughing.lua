----------------------------------------------------------------
-- CP2_Audio.lua
-- Plays the "Laughter" audio slot (Girl_laugh)
----------------------------------------------------------------

local DEBUG = true
local SLOT_ID = "Laughter"   -- ✅ MUST match Slot ID in editor exactly

function on_create(self)
    SHARED.CP2Audio = SHARED.CP2Audio or {}

    function SHARED.CP2Audio.Play()
        if type(Audio_PlaySlot) ~= "function" then
            print("[CP2_Audio] ERROR: Audio_PlaySlot not available")
            return false
        end

        Audio_PlaySlot(EntityID, SLOT_ID)

        if DEBUG then
            print("[CP2_Audio] Play -> slot:", SLOT_ID, "entity:", EntityID)
        end
        return true
    end

    if DEBUG then
        print("[CP2_Audio] Ready (slot=" .. SLOT_ID .. ") on EntityID=" .. tostring(EntityID))
    end
end

function on_destroy(self)
    if SHARED.CP2Audio then
        SHARED.CP2Audio = nil
    end
end
