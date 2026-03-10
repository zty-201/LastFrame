-- sfx_ambience_manager.lua
print("[AmbienceManager] script loaded")

-- Must match inspector Slot IDs exactly
local SLOT_MENU    = "Menu"
local SLOT_OUTDOOR = "Outdoor"
local SLOT_INDOOR  = "Indoor"

-- Tuning
local FADE_SECONDS = 1.0
local STOP_EPS     = 0.05

-- 0 = menu, 1 = outdoor, 2 = indoor
local MODE_MENU    = 0
local MODE_OUTDOOR = 1
local MODE_INDOOR  = 2

local current = 0.0          -- current indoor factor [0..1]
local target  = 0.0          -- target indoor factor [0..1]
local lastMode = nil
local indoorPlaying = false

local _dbgT = 0.0

local function ensureGameState()
    -- If some script reset GAME_STATE, don't let it kill ambience mid-game
    if _G.GAME_STATE == nil then
        _G.GAME_STATE = "MENU"
    end
end

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

local function getMode()
    if _G.GAME_STATE ~= "GAME" then
        return MODE_MENU
    end

    return MODE_OUTDOOR
    -- return isIndoorWanted() and MODE_INDOOR or MODE_OUTDOOR
end

local function applyMode(mode)
    -- MENU MODE
    if mode == MODE_MENU then
        -- no indoor muffling
        current = 0.0
        target  = 0.0
        Audio_SetIndoorFactor(0.0)

        Audio_PlaySlot(EntityID, SLOT_MENU)
        Audio_StopSlot(EntityID, SLOT_OUTDOOR)
        Audio_StopSlot(EntityID, SLOT_INDOOR)

        indoorPlaying = false
        return
    end

    -- GAME MODE (Outdoor / Indoor crossfade)
    Audio_StopSlot(EntityID, SLOT_MENU)

    -- Apply Occlusion
    local curved = 1.0 - (1.0 - current) * (1.0 - current)
    Audio_SetIndoorFactor(curved)

    -- Outdoor always plays during gameplay, never stop
    if not indoorPlaying and target == 0.0 then
        Audio_PlaySlot(EntityID, SLOT_OUTDOOR)
    end

    -- Start indoor when transitioning into indoor
    if target == 1.0 and not indoorPlaying then
        Audio_PlaySlot(EntityID, SLOT_INDOOR)
        indoorPlaying = true
    end

    -- Stop indoor only once fully back outside
    if current <= STOP_EPS and indoorPlaying then
        Audio_StopSlot(EntityID, SLOT_INDOOR)
        indoorPlaying = false
    end

    -- if target >= 0.5 then
    --     Audio_PlaySlot(EntityID, SLOT_INDOOR)
    --     if current >= (1.0 - STOP_EPS) then
    --         Audio_StopSlot(EntityID, SLOT_OUTDOOR)
    --     end
    -- else
    --     Audio_PlaySlot(EntityID, SLOT_OUTDOOR)
    --     if current <= STOP_EPS then
    --         Audio_StopSlot(EntityID, SLOT_INDOOR)
    --     end
    -- end
end

function on_create(self)
    print("[AmbienceManager] on_create EntityID =", EntityID)

    -- if _G.INDOOR_COUNT == nil then
    --     _G.INDOOR_COUNT = 0
    -- end

    -- if _G.GAME_STATE == nil then
    --     _G.GAME_STATE = "MENU"
    -- end

    _G.INDOOR_COUNT = 0
    _G.GAME_STATE = "MENU"

    local mode = getMode()
    lastMode = mode

    FADE_SECONDS = Audio_GetSlotFadeDuration(EntityID, SLOT_OUTDOOR)
    --print("[AmbienceManager] FADE_SECONDS =", FADE_SECONDS)

    if mode == MODE_MENU then
        applyMode(MODE_MENU)
    else
        target  = isIndoorWanted() and 1.0 or 0.0
        current = target

        Audio_PlaySlot(EntityID, SLOT_OUTDOOR)
        if isIndoorWanted() then
            Audio_PlaySlot(EntityID, SLOT_INDOOR)
            indoorPlaying = true
        end
        Audio_SetIndoorFactor(current)
        -- applyMode(mode)
    end
end

function on_update(self, dt)
    if dt == nil then dt = 0.016 end

    _dbgT = _dbgT + dt
    if _dbgT >= 1.0 then
        _dbgT = 0.0
        --print(string.format("[AmbienceManager] GAME_STATE=%s INDOOR_COUNT=%s", tostring(_G.GAME_STATE), tostring(_G.INDOOR_COUNT)))
    end

    local mode = getMode()

    -- Mode changed? snap setup immediately
    if mode ~= lastMode then
        lastMode = mode

        if mode == MODE_MENU then
            applyMode(MODE_MENU)
            return
        end

        -- switching into GAME: start outdoor/indoor system cleanly
        target  = isIndoorWanted() and 1.0 or 0.0
        current = target
        applyMode(mode)
        return
    end

    -- If we are still in menu, nothing to blend
    if mode == MODE_MENU then
        return
    end

    -- Indoor/Outdoor blending during gameplay
    target = isIndoorWanted() and 1.0 or 0.0

    local speed = (FADE_SECONDS <= 0.0) and 9999.0 or (1.0 / FADE_SECONDS)
    local prev = current

    current = moveTowards(current, target, speed * dt)
    current = clamp01(current)

    if math.abs(current - prev) > 0.0001 then
        --print(string.format("[AmbienceManager] current=%.3f target=%.3f speed=%.3f dt=%.4f", current, target, speed, dt))
        applyMode(mode)
    end
end
