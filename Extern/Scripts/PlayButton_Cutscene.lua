print("[PlayButtonController] ===== SCRIPT FILE LOADED =====")

local function try_reload_self(reason)
    if Script_RequestReloadEntity == nil then
        print("[PlayButtonController] Reload requested (" .. tostring(reason) .. ") BUT Script_RequestReloadEntity is NIL (binding missing)")
        return false
    end

    print("[PlayButtonController] Reload requested (" .. tostring(reason) .. ") -> calling Script_RequestReloadEntity(EntityID=" .. tostring(EntityID) .. ")")
    Script_RequestReloadEntity(EntityID)
    return true
end

function on_create(self)
    print("[PlayButtonController] ===== on_create CALLED =====")
    print("[PlayButtonController] EntityID=" .. tostring(EntityID))

    if StateMachine_SetTrigger == nil then
        print("[PlayButtonController] ERROR: StateMachine_SetTrigger is NIL (binding missing)")
    else
        print("[PlayButtonController] StateMachine_SetTrigger exists")
    end
end

function on_update(self, dt)
end

function on_button_PlayButton(self)
    print("[PlayButtonController] ===== PLAY BUTTON PRESSED =====")

    SHARED = SHARED or {}

    -- Hard reset all cutscene / ambience handoff state so replay after restart is clean
    SHARED.CUTSCENE_AUDIO_COMMAND = nil
    SHARED.CUTSCENE_AMBIENCE = nil
    SHARED.GAMEPLAY_BASE_AMBIENCE = nil

    _G.GAME_STATE = "CUTSCENE"
    _G.INDOOR_COUNT = 0
    _G.ROOFTOP_COUNT = 0
    _G.ACTIVE_AMBIENCE_SLOT = nil

    if StateMachine_SetTrigger == nil then
        print("[PlayButtonController] ERROR: StateMachine_SetTrigger binding missing")
        return
    end

    local ok, err = pcall(function()
        StateMachine_SetTrigger("StartIntro")
    end)

    if ok then
        print("[PlayButtonController] Requested MainMenu -> IntroCutscene")
    else
        print("[PlayButtonController] ERROR triggering StartIntro: " .. tostring(err))
    end
end

function on_button_ReloadPlayScript(self)
    print("[PlayButtonController] ===== RELOAD BUTTON PRESSED =====")
    try_reload_self("button ReloadPlayScript")
end