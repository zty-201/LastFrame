-- indoor_audio_detector.lua
print("[IndoorAudio] script file loaded")

local indoorZones = {
    [259] = true,
    [377] = true,
    [378] = true,
    [380] = true,
}

-- Shared across ALL zone scripts
if _G.INDOOR_COUNT == nil then
    _G.INDOOR_COUNT = 0
end

-- Local to THIS zone script instance (prevents double enter/exit)
local playerInsideThisZone = false

local function applyIndoor()
    if _G.INDOOR_COUNT > 0 then
        Audio_SetIndoorFactor(1.0)
    else
        Audio_SetIndoorFactor(0.0)
    end
end

function on_create(self)
    print("[IndoorAudio] on_create, Zone EntityID =", EntityID)
    -- Optional: don't force outdoor here, because other zones might already be active
    -- Just apply based on current count:
    applyIndoor()
end

function on_trigger_enter(self, otherID)
    if SHARED.PlayerID == nil then return end
    if otherID ~= SHARED.PlayerID then return end

    -- Only zones listed as indoor should affect audio
    if not indoorZones[EntityID] then return end

    -- Debounce
    if playerInsideThisZone then return end
    playerInsideThisZone = true

    _G.INDOOR_COUNT = _G.INDOOR_COUNT + 1
    --print(string.format("[IndoorAudio] Player ENTER zone %d | count=%d", EntityID, _G.INDOOR_COUNT))

    applyIndoor()
end

function on_trigger_exit(self, otherID)
    if SHARED.PlayerID == nil then return end
    if otherID ~= SHARED.PlayerID then return end

    if not indoorZones[EntityID] then return end

    -- Debounce
    if not playerInsideThisZone then return end
    playerInsideThisZone = false

    _G.INDOOR_COUNT = math.max(0, _G.INDOOR_COUNT - 1)
    --print(string.format("[IndoorAudio] Player EXIT zone %d | count=%d", EntityID, _G.INDOOR_COUNT))

    applyIndoor()
end
-- -- indoor_audio_detector.lua

-- print("[IndoorAudio] script file loaded")

-- -- Use a Lookup Table (Set) instead of an Array for faster checking
-- -- Format: [TriggerID] = true
-- local indoorZones = { 
--     [259] = true,
--     [377] = true,
--     [378] = true,
--     [380] = true,
--     -- Add more later like this:
--     -- [300] = true, 
--     -- [301] = true, 
-- }

-- function on_create(self)

--     print("[IndoorAudio] on_create, EntityID =", EntityID)
--     print("[Player ID] =", SHARED.PlayerID)
--     -- Ensure we start with outdoor audio
--     Audio_SetIndoorFactor(0.0)
    
-- end

-- function on_trigger_enter(self, otherID)

--     -- 1. Check if the Player has registered yet
--     if SHARED.PlayerID == nil then return end

--     -- 2. Check: Is the entity entering me the Player?
--     if otherID == SHARED.PlayerID then
--         print(" > MATCH! Audio ON.")
--         Audio_SetIndoorFactor(1.0)
--     end

-- end

-- function on_trigger_exit(self, otherID)

--     -- Only turn off audio if the PLAYER leaves
--     if otherID == SHARED.PlayerID then
--         print("[Zone] Player left")
--         Audio_SetIndoorFactor(0.0)
--     end

-- end