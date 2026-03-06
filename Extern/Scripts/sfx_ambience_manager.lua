-- sfx_ambience_manager.lua
print("[AmbienceManager] script loaded")

local activeSlot = nil          -- To track what is currently playing

-- Must match inspector Slot IDs exactly
local SLOT_MENU    = "Menu"
local SLOT_OUTDOOR = "Outdoor"
local SLOT_INDOOR  = "Indoor"

-- Tuning
local FADE_SECONDS = 1.2
-- local STOP_EPS     = 0.05

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
    return isIndoorWanted() and MODE_INDOOR or MODE_OUTDOOR
end

-- local function applyMode(mode)
--     -- MENU MODE
--     if mode == MODE_MENU then
--         -- no indoor muffling
--         current = 0.0
--         target  = 0.0
--         Audio_SetIndoorFactor(0.0)

--         if activeSlot ~= SLOT_MENU then
--             Audio_PlaySlot(EntityID, SLOT_MENU)
--             Audio_StopSlot(EntityID, SLOT_OUTDOOR)
--             Audio_StopSlot(EntityID, SLOT_INDOOR)
--             activeSlot = SLOT_MENU
--         end
--         return
--     end

--     -- GAME MODE (Outdoor / Indoor crossfade)
--     Audio_StopSlot(EntityID, SLOT_MENU)
--     Audio_SetIndoorFactor(current)

--     -- Start both OUTDOOR and INDOOR slots once entering game mode
--     if activeSlot ~= "BOTH" then
--         Audio_PlaySlot(EntityID, SLOT_OUTDOOR)
--         Audio_PlaySlot(EntityID, SLOT_INDOOR)
--         activeSlot = "BOTH"
--     end

--     -- if target >= 0.5 then
--     --     -- Transitioning to INDOOR
--     --     if activeSlot ~= SLOT_INDOOR then
--     --         Audio_PlaySlot(EntityID, SLOT_INDOOR)
--     --         activeSlot = SLOT_INDOOR
--     --     -- Audio_PlaySlot(EntityID, SLOT_INDOOR)
--     --     -- if current >= (1.0 - STOP_EPS) then
--     --     --     Audio_StopSlot(EntityID, SLOT_OUTDOOR)
--     --     end
--     --     if current >= (1.0 - STOP_EPS) then
--     --         Audio_StopSlot(EntityID, SLOT_OUTDOOR)
--     --     end
--     -- else
--     --     -- Transitioning to OUTDOOR
--     --     if activeSlot ~= SLOT_OUTDOOR then
--     --         Audio_PlaySlot(EntityID, SLOT_OUTDOOR)
--     --         activeSlot = SLOT_OUTDOOR
--     --     end
--     --     if current <= STOP_EPS then
--     --         Audio_StopSlot(EntityID, SLOT_INDOOR)
--     --     -- Audio_PlaySlot(EntityID, SLOT_OUTDOOR)
--     --     -- if current <= STOP_EPS then
--     --     --     Audio_StopSlot(EntityID, SLOT_INDOOR)
--     --     end
--     -- end
-- end

local function startGameAmbience()

    -- Outdoor always starts and never stops during gameplay
    Audio_PlaySlot(EntityID, SLOT_OUTDOOR)

    -- If already indoors on create/re-entry, snap and start indoor too
    if isIndoorWanted() then
        current 1.0
        target = 1.0
        Audio_PlaySlot(EntityID, SLOT_INDOOR)
        indoorPlaying = true
    else
        current = 0.0
        target = 0.0
        indoorPlaying = false
    end

    Audio_SetIndoorFactor(current)
end

function on_create(self)
    print("[AmbienceManager] on_create EntityID =", EntityID)

    -- if _G.INDOOR_COUNT == nil then
    --     _G.INDOOR_COUNT = 0
    -- end

    -- if _G.GAME_STATE == nil then
    --     _G.GAME_STATE = "MENU"
    -- end

    -- local mode = getMode()
    -- lastMode = mode

    -- if mode == MODE_MENU then
    --     Audio_SetIndoorFactor(0.0)
    --     Audio_PlaySlot(EntityID, SLOT_MENU)
    --     --applyMode(MODE_MENU)
    -- else
    --     startGameAmbience()
    -- end
end

function on_update(self, dt)
    if dt == nil then dt = 0.016 end

    if not initialized then
        if EntityID == nil then return end
        doInit()
        return
    end

    _dbgT = _dbgT + dt
    if _dbgT >= 1.0 then
        _dbgT = 0.0
        print(string.format(
            "[AmbienceManager] GAME_STATE=%s INDOOR_COUNT=%s",
            tostring(_G.GAME_STATE),
            tostring(_G.INDOOR_COUNT)
        ))
    end

    local mode = getMode()

    -- Mode changed? snap setup immediately
    if mode ~= lastMode then
        lastMode = mode

        if mode == MODE_MENU then
            -- activeSlot = nil
            -- applyMode(MODE_MENU)
            current = 0.0
            target = 0.0
            Audio_SetIndoorFactor(0.0)
            Audio_PlaySlot(EntityID, SLOT_MENU)
            Audio_StopSlot(EntityID, SLOT_OUTDOOR)
            Audio_StopSlot(EntityID, SLOT_INDOOR)
            indoorPlaying = false
            return
        end
            
        -- Switched into GAME from menu
        Audio_StopSlot(EntityID, SLOT_MENU)
        startGameAmbience()
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

    -- Drive the group volumes and DSP every frame
    if math.abs(current - prev) > 0.0001 then
        -- applyMode(mode)
        Audio_SetIndoorFactor(current)
    end

    -- Start indoor slot the moment we begin transitioning indoors
    if target == 1.0 and not indoorPlaying then
        Audio_PlaySlot(EntityID, SLOT_INDOOR)
        indoorPlaying = true
    end

    -- Stop indoor once fully outdoor
    if current <= 0.0 and indoorPlaying then
        Audio_StopSlot(EntityID, SLOT_INDOOR)
        indoorPlaying = false
    end
end
