-- indoor_audio_detector.lua
-- Attach to PLAYER entity.

print("[IndoorAudio] script file loaded")

-- Use a Lookup Table (Set) instead of an Array for faster checking
-- Format: [TriggerID] = true
local indoorZones = { 
    [259] = true,
    -- Add more later like this:
    -- [300] = true, 
    -- [301] = true, 
}
--local currentZoneCount = 0  -- Use a counter to handle overlapping zones (e.g., walking from Hallway -> Room without audio cutting out)

function on_create(self)
    print("[IndoorAudio] on_create, EntityID =", EntityID)
    print("[Player ID] =", SHARED.PlayerID)
    -- Ensure we start with outdoor audio
    Audio_SetIndoorFactor(0.0)
end

function on_trigger_enter(self, otherID)
    -- 1. Check if the Player has registered yet
    if SHARED.PlayerID == nil then return end
    -- 2. Check: Is the entity entering me the Player?
    if otherID == SHARED.PlayerID then
        print(" > MATCH! Audio ON.")
        Audio_SetIndoorFactor(1.0)
    --else
        --print(" > MISMATCH! (Child object?)")
    end
end

function on_trigger_exit(self, otherID)
    -- Only turn off audio if the PLAYER leaves
    if otherID == SHARED.PlayerID then
        print("[Zone] Player left")
        Audio_SetIndoorFactor(0.0)
    end
end

-- local indoors = false

-- -- check rate (10 times/sec)
-- local timer = 0.0
-- local interval = 0.1

-- local function isInsideAnyZone(playerId)
--     for i = 1, #indoorZones do
--         if Physics_CheckCollision(playerId, indoorZones[i]) then
--             return true
--         end
--     end
--     return false
-- end

-- function on_create(self)
--     print("[IndoorAudio] on_create, EntityID =", EntityID)
-- end

-- function on_update(self, dt)
--     print("[IndoorAudio] inside=", tostring(Physics_CheckCollision(EntityID, 259)))
--     timer = timer + dt
--     if timer < interval then return end
--     timer = 0.0

--     local inside = isInsideAnyZone(EntityID)

--     if inside ~= indoors then
--         indoors = inside
--         if indoors then
--             print("[IndoorAudio] ENTER indoor zone")
--             Audio_SetIndoorFactor(1.0)
--         else
--             print("[IndoorAudio] EXIT indoor zone")
--             Audio_SetIndoorFactor(0.0)
--         end
--     end
-- end