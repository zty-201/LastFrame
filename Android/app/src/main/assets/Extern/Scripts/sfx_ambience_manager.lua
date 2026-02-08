-- sfx_ambience_manager.lua
print("[AmbienceManager] script loaded")

-- Must match inspector Slot IDs exactly
local SLOT_OUTDOOR = "Outdoor"
local SLOT_INDOOR  = "Indoor"

-- Tuning
local FADE_SECONDS = 1.2     -- transition duration outdoor<->indoor
local STOP_EPS     = 0.05    -- near-0/near-1 threshold for stopping the other loop

local current = 0.0          -- current indoor factor [0..1]
local target  = 0.0          -- target indoor factor [0..1]
local lastState = nil

local function clamp01(x)
    if x < 0.0 then return 0.0 end
    if x > 1.0 then return 1.0 end
    return x
end

local function moveTowards(curr, tgt, maxDelta)
    if curr < tgt then
        return math.min(curr + maxDelta, tgt)
    else
        return math.max(curr - maxDelta, tgt)
    end
end

local function isIndoorWanted()
    return (_G.INDOOR_COUNT ~= nil and _G.INDOOR_COUNT > 0)
end

local function applyState()

    -- Smooth indoor/outdoor muffling
    Audio_SetIndoorFactor(current)

    local state = (target >= 0.5) and "INDOOR" or "OUTDOOR"

    -- DEBUG: print ONLY when state changes
    if lastState ~= state then
        print(string.format(
            "[AmbienceManager] Transition -> %s | IndoorCount=%d",
            state,
            _G.INDOOR_COUNT or 0
        ))
        lastState = state
    end

    -- Handle slot playback
    if target >= 0.5 then
        
        Audio_PlaySlot(EntityID, SLOT_INDOOR)

        if current >= (1.0 - STOP_EPS) then
            Audio_StopSlot(EntityID, SLOT_OUTDOOR)
        end

    else
        
        Audio_PlaySlot(EntityID, SLOT_OUTDOOR)

        if current <= STOP_EPS then
            Audio_StopSlot(EntityID, SLOT_INDOOR)
        end
    end
end


function on_create(self)
    print("[AmbienceManager] on_create EntityID =", EntityID)

    if _G.INDOOR_COUNT == nil then
        _G.INDOOR_COUNT = 0
    end

    target = isIndoorWanted() and 1.0 or 0.0
    current = target

    -- Start in correct state
    if target >= 0.5 then
        Audio_PlaySlot(EntityID, SLOT_INDOOR)
        Audio_StopSlot(EntityID, SLOT_OUTDOOR)
    else
        Audio_PlaySlot(EntityID, SLOT_OUTDOOR)
        Audio_StopSlot(EntityID, SLOT_INDOOR)
    end

    Audio_SetIndoorFactor(current)
end

function on_update(self, dt)
    if dt == nil then dt = 0.016 end

    target = isIndoorWanted() and 1.0 or 0.0

    local speed = (FADE_SECONDS <= 0.0) and 9999.0 or (1.0 / FADE_SECONDS)
    local prev = current

    current = moveTowards(current, target, speed * dt)
    current = clamp01(current)

    if math.abs(current - prev) > 0.0001 then
        applyState()
    end
end
