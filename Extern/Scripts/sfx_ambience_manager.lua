print("[AmbienceManager] script loaded")

-- Must match inspector Slot IDs exactly
local SLOT_MENU    = "Menu"
local SLOT_OUTDOOR = "Outdoor"
local SLOT_INDOOR  = "Indoor"
local SLOT_RAIN    = "AmbienceRain"

local MODE_MENU    = 0
local MODE_OUTDOOR = 1
local MODE_INDOOR  = 2

local lastMode = nil
local rainPlaying = false

local _dbgT = 0.0

local function isIndoorWanted()
    return (_G.INDOOR_COUNT ~= nil and _G.INDOOR_COUNT > 0)
end

local function isRooftopWanted()
    return (_G.ROOFTOP_COUNT ~= nil and _G.ROOFTOP_COUNT > 0)
end

local function getMode()
    if _G.GAME_STATE ~= "GAME" then
        return MODE_MENU
    end

    if isIndoorWanted() then
        return MODE_INDOOR
    end

    return MODE_OUTDOOR
end

local function applyBaseMode(mode)
    if mode == MODE_MENU then
        Audio_PlaySlot(EntityID, SLOT_MENU)
        Audio_StopSlot(EntityID, SLOT_OUTDOOR)
        Audio_StopSlot(EntityID, SLOT_INDOOR)
        Audio_StopSlot(EntityID, SLOT_RAIN)
        rainPlaying = false
        return
    end

    -- Stop menu once in gameplay
    Audio_StopSlot(EntityID, SLOT_MENU)

    if mode == MODE_OUTDOOR then
        Audio_PlaySlot(EntityID, SLOT_OUTDOOR)
        Audio_StopSlot(EntityID, SLOT_INDOOR)
    elseif mode == MODE_INDOOR then
        Audio_PlaySlot(EntityID, SLOT_INDOOR)
        Audio_StopSlot(EntityID, SLOT_OUTDOOR)
    end
end

local function applyRainLayer()
    local wantRain = (_G.GAME_STATE == "GAME") and isRooftopWanted()

    if wantRain then
        if not rainPlaying then
            Audio_PlaySlot(EntityID, SLOT_RAIN)
            rainPlaying = true
            print("[AmbienceManager] Rain ON")
        end
    else
        if rainPlaying then
            Audio_StopSlot(EntityID, SLOT_RAIN)
            rainPlaying = false
            print("[AmbienceManager] Rain OFF")
        end
    end
end

function on_create(self)
    print("[AmbienceManager] on_create EntityID =", EntityID)

    _G.INDOOR_COUNT = 0
    _G.ROOFTOP_COUNT = 0
    _G.GAME_STATE = "MENU"

    lastMode = getMode()
    applyBaseMode(lastMode)
    applyRainLayer()
end

function on_update(self, dt)
    if dt == nil then dt = 0.016 end

    _dbgT = _dbgT + dt
    if _dbgT >= 1.0 then
        _dbgT = 0.0
        -- print(string.format("[AmbienceManager] GAME_STATE=%s INDOOR=%d ROOFTOP=%d",
        --     tostring(_G.GAME_STATE), tonumber(_G.INDOOR_COUNT or 0), tonumber(_G.ROOFTOP_COUNT or 0)))
    end

    local mode = getMode()

    if mode ~= lastMode then
        lastMode = mode
        applyBaseMode(mode)

        if mode == MODE_OUTDOOR then
            print("[AmbienceManager] Switched to OUTDOOR")
        elseif mode == MODE_INDOOR then
            print("[AmbienceManager] Switched to INDOOR")
        elseif mode == MODE_MENU then
            print("[AmbienceManager] Switched to MENU")
        end
    end

    applyRainLayer()
end