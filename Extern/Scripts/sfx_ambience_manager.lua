print("[AmbienceManager] script loaded")

local SLOT_MENU    = "Menu"
local SLOT_OUTDOOR = "Outdoor"
local SLOT_INDOOR  = "Indoor"
local SLOT_RAIN    = "AmbienceRain"

local MODE_MENU     = 0
local MODE_OUTDOOR  = 1
local MODE_INDOOR   = 2
local MODE_CUTSCENE = 3

local lastMode = nil
local rainPlaying = false
local cutsceneAudioStarted = false

local ambienceOverride = nil
local gameplayBaseOverride = nil

local _dbgT = 0.0

local function isIndoorWanted()
    return (_G.INDOOR_COUNT ~= nil and _G.INDOOR_COUNT > 0)
end

local function isRooftopWanted()
    return (_G.ROOFTOP_COUNT ~= nil and _G.ROOFTOP_COUNT > 0)
end

local function has_cutscene_api()
    return type(rawget(_G, "UI_IsPseudoCutscenePlaying")) == "function"
        or type(rawget(_G, "UI_IsCutscenePlaying")) == "function"
end

local function is_cutscene_playing()
    local checkFns = {
        "UI_IsPseudoCutscenePlaying",
        "UI_IsCutscenePlaying",
    }

    for i = 1, #checkFns do
        local fn = rawget(_G, checkFns[i])
        if type(fn) == "function" then
            local ok, result = pcall(fn)
            if ok then
                return result == true
            end
        end
    end

    return false
end

local function request_cutscene_audio_start()
    SHARED = SHARED or {}
    SHARED.CUTSCENE_AUDIO_COMMAND = "START"
    cutsceneAudioStarted = true
    print("[AmbienceManager] Requested CUTSCENE audio START")
end

local function request_cutscene_audio_stop()
    SHARED = SHARED or {}
    SHARED.CUTSCENE_AUDIO_COMMAND = "STOP"
    cutsceneAudioStarted = false
    print("[AmbienceManager] Requested CUTSCENE audio STOP")
end

local function getMode()
    if _G.GAME_STATE == "CUTSCENE" then
        return MODE_CUTSCENE
    end

    if _G.GAME_STATE ~= "GAME" then
        return MODE_MENU
    end

    if isIndoorWanted() then
        return MODE_INDOOR
    end

    return MODE_OUTDOOR
end

local function applyAmbienceOverride()
    local targetSlot = nil

    if ambienceOverride == "INDOOR" then
        targetSlot = SLOT_INDOOR
    elseif ambienceOverride == "OUTDOOR" then
        targetSlot = SLOT_OUTDOOR
    else
        return false
    end

    if _G.ACTIVE_AMBIENCE_SLOT ~= targetSlot then
        Audio_StopSlot(EntityID, SLOT_MENU)
        Audio_StopSlot(EntityID, SLOT_OUTDOOR)
        Audio_StopSlot(EntityID, SLOT_INDOOR)
        Audio_StopSlot(EntityID, SLOT_RAIN)

        Audio_PlaySlot(EntityID, targetSlot)

        _G.ACTIVE_AMBIENCE_SLOT = targetSlot
        rainPlaying = false

        print("[AmbienceManager] Applied ambience override slot = " .. tostring(targetSlot))
    end

    return true
end

local function applyGameplayBaseOverride()
    local targetSlot = nil

    -- Indoor triggerboxes should always win during gameplay
    if isIndoorWanted() then
        targetSlot = SLOT_INDOOR
    elseif gameplayBaseOverride == "OUTDOOR" then
        targetSlot = SLOT_OUTDOOR
    elseif gameplayBaseOverride == "INDOOR" then
        targetSlot = SLOT_INDOOR
    else
        return false
    end

    if _G.ACTIVE_AMBIENCE_SLOT ~= targetSlot then
        Audio_StopSlot(EntityID, SLOT_MENU)
        Audio_StopSlot(EntityID, SLOT_OUTDOOR)
        Audio_StopSlot(EntityID, SLOT_INDOOR)
        Audio_PlaySlot(EntityID, targetSlot)

        _G.ACTIVE_AMBIENCE_SLOT = targetSlot
        print("[AmbienceManager] Forced gameplay ambience = " .. tostring(targetSlot))
    end

    return true
end

local function applyBaseMode(mode)
    if mode == MODE_MENU then
        Audio_PlaySlot(EntityID, SLOT_MENU)
        Audio_StopSlot(EntityID, SLOT_OUTDOOR)
        Audio_StopSlot(EntityID, SLOT_INDOOR)
        Audio_StopSlot(EntityID, SLOT_RAIN)
        rainPlaying = false

        if cutsceneAudioStarted then
            request_cutscene_audio_stop()
        end
        return
    end

    if mode == MODE_CUTSCENE then
        Audio_StopSlot(EntityID, SLOT_MENU)
        Audio_StopSlot(EntityID, SLOT_OUTDOOR)
        Audio_StopSlot(EntityID, SLOT_INDOOR)
        Audio_StopSlot(EntityID, SLOT_RAIN)
        rainPlaying = false
        return
    end

    Audio_StopSlot(EntityID, SLOT_MENU)

    if applyGameplayBaseOverride() then
        if cutsceneAudioStarted then
            request_cutscene_audio_stop()
        end
        return
    end

    if mode == MODE_OUTDOOR then
        Audio_PlaySlot(EntityID, SLOT_OUTDOOR)
        Audio_StopSlot(EntityID, SLOT_INDOOR)
    elseif mode == MODE_INDOOR then
        Audio_PlaySlot(EntityID, SLOT_INDOOR)
        Audio_StopSlot(EntityID, SLOT_OUTDOOR)
    end

    if cutsceneAudioStarted then
        request_cutscene_audio_stop()
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

    SHARED = SHARED or {}
    SHARED.CUTSCENE_AUDIO_COMMAND = nil
    SHARED.CUTSCENE_AMBIENCE = nil
    SHARED.GAMEPLAY_BASE_AMBIENCE = nil

    ambienceOverride = nil
    gameplayBaseOverride = nil
    cutsceneAudioStarted = false
    lastMode = getMode()
    applyBaseMode(lastMode)
    applyRainLayer()
end

function on_update(self, dt)
    if dt == nil then dt = 0.016 end
    SHARED = SHARED or {}

    _dbgT = _dbgT + dt
    if _dbgT >= 1.0 then
        _dbgT = 0.0

        local apiPlaying = false
        if has_cutscene_api() then
            apiPlaying = is_cutscene_playing()
        end

        print("[AmbienceManager][DBG] GAME_STATE=" .. tostring(_G.GAME_STATE)
            .. " mode=" .. tostring(lastMode)
            .. " apiPlaying=" .. tostring(apiPlaying)
            .. " cutsceneAudioStarted=" .. tostring(cutsceneAudioStarted)
            .. " ambienceOverride=" .. tostring(ambienceOverride)
            .. " gameplayBaseOverride=" .. tostring(gameplayBaseOverride))
    end

    if SHARED.CUTSCENE_AMBIENCE ~= nil then
        ambienceOverride = SHARED.CUTSCENE_AMBIENCE
        SHARED.CUTSCENE_AMBIENCE = nil
        print("[AmbienceManager] ambienceOverride -> " .. tostring(ambienceOverride))
    end

    if SHARED.GAMEPLAY_BASE_AMBIENCE ~= nil then
        gameplayBaseOverride = SHARED.GAMEPLAY_BASE_AMBIENCE
        SHARED.GAMEPLAY_BASE_AMBIENCE = nil
        print("[AmbienceManager] gameplayBaseOverride -> " .. tostring(gameplayBaseOverride))
    end

    local mode = getMode()
    local modeChanged = (mode ~= lastMode)

    if modeChanged then
        -- Hard reset when entering a fresh cutscene so replay-after-quit works
        if mode == MODE_CUTSCENE then
            cutsceneAudioStarted = false
            ambienceOverride = nil
            gameplayBaseOverride = nil
            _G.ACTIVE_AMBIENCE_SLOT = nil
            SHARED.CUTSCENE_AUDIO_COMMAND = nil
            SHARED.CUTSCENE_AMBIENCE = nil
            SHARED.GAMEPLAY_BASE_AMBIENCE = nil
            print("[AmbienceManager] Fresh CUTSCENE entry reset")
        end

        -- Clean menu state as well
        if mode == MODE_MENU then
            cutsceneAudioStarted = false
            ambienceOverride = nil
            gameplayBaseOverride = nil
            _G.ACTIVE_AMBIENCE_SLOT = nil
        end

        lastMode = mode
        applyBaseMode(mode)

        if mode == MODE_OUTDOOR then
            print("[AmbienceManager] Switched to OUTDOOR")
        elseif mode == MODE_INDOOR then
            print("[AmbienceManager] Switched to INDOOR")
        elseif mode == MODE_MENU then
            print("[AmbienceManager] Switched to MENU")
        elseif mode == MODE_CUTSCENE then
            print("[AmbienceManager] Switched to CUTSCENE")
        end
    end

    if mode == MODE_CUTSCENE then
        if not cutsceneAudioStarted then
            request_cutscene_audio_start()
            applyAmbienceOverride()
            return
        end

        applyAmbienceOverride()

        if has_cutscene_api() then
            local playing = is_cutscene_playing()
            if not playing then
                print("[AmbienceManager] Cutscene ended or skipped -> switching GAME_STATE to GAME")

                request_cutscene_audio_stop()

                gameplayBaseOverride = "OUTDOOR"
                ambienceOverride = nil
                _G.ACTIVE_AMBIENCE_SLOT = nil

                _G.GAME_STATE = "GAME"
                lastMode = nil
                return
            end
        end

        return
    end

    if mode == MODE_OUTDOOR or mode == MODE_INDOOR then
        if applyGameplayBaseOverride() then
            applyRainLayer()
            return
        end
    end

    applyRainLayer()
end